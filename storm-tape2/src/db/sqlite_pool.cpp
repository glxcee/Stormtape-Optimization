#include "sqlite_pool.hpp"
#include <chrono>
#include <thread>
#include <iostream>

namespace storm::db {

SqlitePool::SqlitePool(std::string const& db_path, std::size_t n_sessions)
  : m_db_path(db_path)
{
    if (n_sessions == 0) n_sessions = 2;
    m_sessions_storage.reserve(n_sessions);
    for (std::size_t i = 0; i < n_sessions; ++i) {
        auto sess = std::make_unique<soci::session>(soci::sqlite3, m_db_path);
        // important PRAGMA for concurrency
        (*sess) << "PRAGMA journal_mode = WAL;";
        (*sess) << "PRAGMA synchronous = NORMAL;";
        // optionally: busy_timeout
        (*sess) << "PRAGMA busy_timeout = 5000;";
        m_available.push_back(sess.get());
        m_sessions_storage.push_back(std::move(sess));
    }
}

SqlitePool::~SqlitePool()
{
    // ensure all sessions destroyed after container cleared
    std::unique_lock lock(m_mutex);
    m_available.clear();
    m_sessions_storage.clear();
}

soci::session* SqlitePool::acquire()
{
    std::unique_lock lock(m_mutex);
    m_cv.wait(lock, [&]{ return !m_available.empty(); });
    auto p = m_available.front();
    m_available.pop_front();
    return p;
}

void SqlitePool::release(soci::session* p)
{
    {
        std::lock_guard lock(m_mutex);
        m_available.push_back(p);
    }
    m_cv.notify_one();
}

std::size_t SqlitePool::size() const
{
    std::lock_guard lock(m_mutex);
    return m_sessions_storage.size();
}

} // namespace storm::db
