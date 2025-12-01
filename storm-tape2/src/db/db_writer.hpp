#pragma once
#include <future>
#include <utility>
#include <deque>
#include <soci/soci.h>
#include <functional>
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace storm::db {

class DbWriter {
public:
    explicit DbWriter(std::string const& db_path);
    ~DbWriter();

    void enqueue(std::function<void(soci::session&)> op);
    void enqueue_and_wait(std::function<void(soci::session&)> op);

    void stop();

private:
    void thread_main();

    // *** ORDINE CORRETTO ***
    std::string m_db_path;

    std::atomic<bool> m_running{true};

    std::recursive_mutex m_mutex;
    std::condition_variable_any m_cv;

    std::deque<
        std::pair<
            std::function<void(soci::session&)>,
            std::shared_ptr<std::promise<void>>
        >
    > m_queue;

    std::thread m_thread;
};


} // namespace storm::db
