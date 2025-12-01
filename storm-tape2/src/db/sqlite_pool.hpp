#pragma once

#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <deque>

namespace storm::db {

class SqlitePool {
public:
    SqlitePool(std::string const& db_path, std::size_t n_sessions);
    ~SqlitePool();

    // Acquire a session pointer. Caller must not delete it.
    // Returns a raw pointer valid until release() is called.
    soci::session* acquire();
    void release(soci::session*);

    std::size_t size() const;

private:
    std::string m_db_path;
    std::vector<std::unique_ptr<soci::session>> m_sessions_storage;
    std::deque<soci::session*> m_available;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
};
} // namespace storm::db
