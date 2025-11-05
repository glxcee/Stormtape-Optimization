// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#include "fixture.t.hpp"
#include "extended_attributes.hpp"
#include "types.hpp"
#include "uuid_generator.hpp"

#include <fmt/std.h>
#include <soci/sqlite3/soci-sqlite3.h>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <string>

namespace storm {

auto random_directory_path(UuidGenerator& uuid_gen)
{
  return fs::temp_directory_path() / uuid_gen();
};

storm::Files generate_random_files(PhysicalPath const& root,
                                   LogicalPath const& access_point,
                                   std::size_t size, UuidGenerator& uuid_gen)
{
  auto generator = [&]() {
    fs::path const filename = fmt::format("storm-{}.dat", uuid_gen());
    return File{LogicalPath{access_point / filename},
                PhysicalPath{root / filename}};
  };
  Files files;
  files.reserve(size);
  std::generate_n(std::back_inserter(files), size, generator);
  return files;
}

static auto create_file(PhysicalPath const& path, const char* size = "1M")
{
  auto const cmd = fmt::format(
      "dd if=/dev/random bs={} count=1 of={} &> /dev/null", size, path.c_str());
  std::system(cmd.c_str());
  set_xattr(path, XAttrName{"user.storm.migrated"}, XAttrValue{""});
  return path;
};

static auto create_stub(PhysicalPath const& path, const char* size = "1M")
{
  auto const cmd = fmt::format(
      "dd if=/dev/zero conv=sparse bs={} count=1 of={} &> /dev/null", size,
      path.c_str());
  std::system(cmd.c_str());
  set_xattr(path, XAttrName{"user.storm.migrated"}, XAttrValue{""});
  return path;
};

Configuration create_config(PhysicalPath const& root,
                            LogicalPath const& access_point)
{
  auto constexpr conf = R"(
storage-areas:
- name: sa1
  root: {}
  access-point: {}
)";
  fs::create_directory(root);
  std::istringstream iss{fmt::format(conf, root, access_point)};
  return load_configuration(iss);
}

TestFixture::TestFixture()
    : m_root(random_directory_path(m_uuid_gen))
    , m_access_point{"/atlas"}
    , m_sql{soci::sqlite3, DB_NAME}
    , m_config{create_config(m_root, m_access_point)}
    , m_storage{}
    , m_db{m_sql}
    , m_service{m_config, m_db, m_storage}
    , m_files{generate_random_files(m_root, m_access_point, 2, m_uuid_gen)}
{}

TestFixture::~TestFixture()
{
  fs::remove_all(m_root);
}

TapeService& TestFixture::get_service()
{
  return m_service;
}

SociDatabase const& TestFixture::get_db() const
{
  return m_db;
}

Files const& TestFixture::get_files() const
{
  return m_files;
}

storm::PhysicalPath TestFixture::create_file_on_disk_at(std::size_t index)
{
  assert(index < m_files.size());
  return create_file(m_files[index].physical_path);
}

storm::PhysicalPath TestFixture::create_stub_on_disk_at(std::size_t index)
{
  assert(index < m_files.size());
  return create_stub(m_files[index].physical_path);
}

} // namespace storm
