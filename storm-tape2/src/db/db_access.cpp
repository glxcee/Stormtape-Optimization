#include "db_access.hpp"

namespace storm::db {

DbAccess::DbAccess(std::string const& db_path,
                   std::size_t n_read_sessions,
                   std::size_t /*writer_sessions*/)
  : m_read_pool(db_path, n_read_sessions), m_writer(db_path)
{}

DbAccess::~DbAccess()
{
    m_writer.stop();
}



template <typename Fn>
auto DbAccess::with_read(Fn&& f) -> decltype(f(std::declval<soci::session&>())) {
    // Note: this template must be defined in header to be visible to callers;
    // We'll provide an inline wrapper in the header instead.
    return {};
}

void DbAccess::enqueue_write(std::function<void(soci::session&)> op)
{
    m_writer.enqueue(std::move(op));
}

void DbAccess::enqueue_write_and_wait(std::function<void(soci::session&)> op)
{
    m_writer.enqueue_and_wait(std::move(op));
}

std::size_t DbAccess::read_pool_size() const { return m_read_pool.size(); }

} // namespace storm::db
