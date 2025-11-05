// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#include "configuration.hpp"
#include "database_soci.hpp"
#include "local_storage.hpp"
#include "storage.hpp"
#include "tape_service.hpp"
#include "uuid_generator.hpp"

namespace storm {

auto static constexpr DB_NAME = ":memory:";

class TestFixture
{
  UuidGenerator m_uuid_gen;
  PhysicalPath m_root;
  LogicalPath m_access_point;
  soci::session m_sql;
  Configuration m_config;
  LocalStorage m_storage;
  SociDatabase m_db;
  TapeService m_service;
  Files m_files;

 public:
  TestFixture();
  ~TestFixture();
  TestFixture(TestFixture const&)            = delete;
  TestFixture& operator=(TestFixture const&) = delete;
  TestFixture(TestFixture&&)                 = default;
  TestFixture& operator=(TestFixture&&)      = default;

  TapeService& get_service();
  SociDatabase const& get_db() const;
  Files const& get_files() const;
  storm::PhysicalPath create_file_on_disk_at(std::size_t index);
  storm::PhysicalPath create_stub_on_disk_at(std::size_t index);
};
} // namespace storm
