#include "db_writer.hpp"
#include <soci/sqlite3/soci-sqlite3.h>
#include <future>
#include <iostream>

namespace storm::db {

DbWriter::DbWriter(std::string const& db_path)
  : m_db_path(db_path), m_running(true), m_thread(&DbWriter::thread_main, this)
{}

DbWriter::~DbWriter()
{
    stop();
    if (m_thread.joinable()) m_thread.join();
}

void DbWriter::enqueue(std::function<void(soci::session&)> op)
{
    auto dummy = std::make_shared<std::promise<void>>(); // unused
    {
        std::lock_guard lock(m_mutex);
        m_queue.emplace_back(std::move(op), dummy);
    }
    m_cv.notify_one();
}

void DbWriter::enqueue_and_wait(std::function<void(soci::session&)> op)
{
    auto p = std::make_shared<std::promise<void>>();
    {
        std::lock_guard lock(m_mutex);
        m_queue.emplace_back(std::move(op), p);
    }
    m_cv.notify_one();
    // wait
    p->get_future().get();
}

void DbWriter::stop()
{
    {
        std::lock_guard<std::recursive_mutex> lk(m_mutex);
        m_running = false;
        m_cv.notify_all();
    }
    if (m_thread.joinable())
        m_thread.join();
}


void DbWriter::thread_main()
{
    try {
        soci::session sql(soci::sqlite3, m_db_path);
        sql << "PRAGMA journal_mode = WAL;";
        sql << "PRAGMA synchronous = NORMAL;";
        sql << "PRAGMA busy_timeout = 5000;";

        while (m_running) {
            std::pair<std::function<void(soci::session&)>, std::shared_ptr<std::promise<void>>> item;
            {
                std::unique_lock lock(m_mutex);
                if (m_queue.empty()) {
                    m_cv.wait(lock, [&]{ return !m_running || !m_queue.empty(); });
                }
                if (!m_queue.empty()) {
                    item = std::move(m_queue.front());
                    m_queue.pop_front();
                } else if (!m_running) {
                    break;
                } else {
                    continue;
                }
            } // unlock
            try {
                // execute operation inside a transaction for atomicity
                soci::transaction tr(sql);
                item.first(sql);
                tr.commit();
                if (item.second) item.second->set_value();
            } catch (std::exception const& e) {
                // propagate failure via promise exception if waiting
                if (item.second) item.second->set_exception(std::current_exception());
                // log and continue
                std::cerr << "DB writer exception: " << e.what() << std::endl;
            }
        }

        // flush remaining items
        std::deque<decltype(m_queue)::value_type> pending;
        {
            std::lock_guard lock(m_mutex);
            pending = std::move(m_queue);
            m_queue.clear();
        }
        for (auto &it : pending) {
            try {
                soci::transaction tr(sql);
                it.first(sql);
                tr.commit();
                if (it.second) it.second->set_value();
            } catch (...) {
                if (it.second) it.second->set_exception(std::current_exception());
            }
        }

    } catch (std::exception const& e) {
        std::cerr << "DbWriter startup exception: " << e.what() << std::endl;
    }
}

} // namespace storm::db
