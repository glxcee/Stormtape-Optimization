#pragma once

#include "sqlite_pool.hpp"
#include "db_writer.hpp"
#include <functional>
#include <optional>

namespace storm::db {

class DbAccess {
public:
    DbAccess(std::string const& db_path,
             std::size_t n_read_sessions = 4,
             std::size_t writer_sessions = 1);

    ~DbAccess();

    // Read: execute F(soci::session&) and return result
    template <typename Fn>
    auto with_read(Fn&& f) -> decltype(f(std::declval<soci::session&>()));

    // Enqueue a write (async)
    void enqueue_write(std::function<void(soci::session&)> op);

    // Enqueue a write and wait until finished (sync)
    void enqueue_write_and_wait(std::function<void(soci::session&)> op);

    std::size_t read_pool_size() const;

private:
    SqlitePool m_read_pool;
    DbWriter m_writer;
};

} // namespace storm::db
