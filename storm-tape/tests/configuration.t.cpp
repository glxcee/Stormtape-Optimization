// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#include "configuration.hpp"
#include "uuid_generator.hpp"
#include <doctest/doctest.h>
#include <fmt/std.h>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace storm {

class TempDirectory
{
  fs::path m_path;

  static auto random_directory_path()
  {
    storm::UuidGenerator uuid_gen;
    return fs::temp_directory_path() / uuid_gen();
  }

 public:
  TempDirectory()
      : m_path{random_directory_path()}
  {
    fs::create_directory(m_path);
  }
  ~TempDirectory()
  {
    fs::remove_all(m_path);
  }

  fs::path const& path() const
  {
    return m_path;
  }
};
} // namespace storm

TEST_SUITE_BEGIN("Configuration");

TEST_CASE("The storage-areas entry must exist")
{
  auto constexpr conf = R"()";
  std::istringstream is{conf};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "no 'storage-areas' entry in configuration",
                       std::runtime_error);
}

TEST_CASE("The storage-areas entry cannot be empty")
{
  auto constexpr conf = R"(
storage-areas:
)";
  std::istringstream is{conf};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "configuration error - empty 'storage-areas' entry",
                       std::runtime_error);
}

TEST_CASE("A storage area must have a name")
{
  auto constexpr conf = R"(
storage-areas:
- root: {}
  access-point: /someexp
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       R"(there is a storage area with no name)",
                       std::runtime_error);
}

TEST_CASE("The name field of a storage area cannot be empty")
{
  auto constexpr conf = R"(
storage-areas:
- name:
  root: {}
  access-point: /someexp
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       R"(there is a storage area with an empty name)",
                       std::runtime_error);
}

TEST_CASE("The name of a storage area cannot be the empty string")
{
  auto constexpr conf = R"(
storage-areas:
- name: ""
  root: {}
  access-point: /someexp
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       R"(there is a storage area with an empty string name)",
                       std::runtime_error);
}

TEST_CASE("The name of a storage area must be valid")
{
  auto constexpr conf = R"(
storage-areas:
- name: 7up
  root: {}
  access-point: /someexp
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       R"(invalid storage area name '7up')",
                       std::runtime_error);
}

TEST_CASE("Two storage areas cannot have the same name")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
- name: test
  root: {}
  access-point: /someexp2
)";
  storm::TempDirectory tmp1{};
  storm::TempDirectory tmp2{};
  std::istringstream is{fmt::format(conf, tmp1.path(), tmp2.path())};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       R"(two storage areas have the same name 'test')",
                       std::runtime_error);
}

TEST_CASE("A storage area must have an access-point")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       R"(storage area 'test' has no access-point)",
                       std::runtime_error);
}

TEST_CASE("A storage area must have at least one access-point")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point:
)";

  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       R"(storage area 'test' has an empty access-point)",
                       std::runtime_error);
}

TEST_CASE("A single access point for a storage area can be expressed in "
          "multiple ways")
{
  auto constexpr conf = R"(
storage-areas:
- name: test1
  root: {0}
  access-point: /data1
- name: test2
  root: {0}
  access-point:
    - /data2
- name: test3
  root: {0}
  access-point: [ "/data3" ]
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  auto const configuration = storm::load_configuration(is);
  REQUIRE_EQ(configuration.storage_areas.size(), 3);
  auto const& sa1 = configuration.storage_areas[0];
  auto const& sa2 = configuration.storage_areas[1];
  auto const& sa3 = configuration.storage_areas[2];
  CHECK_EQ(sa1.access_points, storm::LogicalPaths{"/data1"});
  CHECK_EQ(sa2.access_points, storm::LogicalPaths{"/data2"});
  CHECK_EQ(sa3.access_points, storm::LogicalPaths{"/data3"});
}

TEST_CASE("The access point of a storage area can be the filesystem root")
{
  auto constexpr conf = R"(
storage-areas:
- name: atlas
  root: {}
  access-point: /
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  auto configuration = storm::load_configuration(is);
  CHECK_EQ(configuration.storage_areas[0].access_points,
           storm::LogicalPaths{"/"});
}

TEST_CASE("A storage area can have many access points")
{
  auto constexpr conf = R"(
storage-areas:
- name: atlas
  root: {0}
  access-point:
    - /atlas1
    - /atlas2
- name: cms
  root: {0}
  access-point: ["/cms1", "/cms2", "/cms3"]
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  auto configuration = storm::load_configuration(is);
  REQUIRE_EQ(configuration.storage_areas.size(), 2);
  auto const& atlas = configuration.storage_areas[0];
  auto const& cms   = configuration.storage_areas[1];
  CHECK_EQ(atlas.access_points, storm::LogicalPaths{"/atlas1", "/atlas2"});
  CHECK_EQ(cms.access_points, storm::LogicalPaths{"/cms1", "/cms2", "/cms3"});
}

TEST_CASE("The access points of a storage area are locally unique")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point:
    - /atlas1
    - /atlas2
    - /atlas1
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(
      storm::load_configuration(is),
      R"(storage areas 'test' and 'test' have the access point '/atlas1' in common)",
      std::runtime_error);
}

TEST_CASE("Two storage areas cannot have the same access point")
{
  auto constexpr conf = R"(
storage-areas:
- name: test1
  root: {}
  access-point: /someexp
- name: test2
  root: {}
  access-point: /someexp
)";
  storm::TempDirectory tmp1{};
  storm::TempDirectory tmp2{};
  std::istringstream is{fmt::format(conf, tmp1.path(), tmp2.path())};
  CHECK_THROWS_WITH_AS(
      storm::load_configuration(is),
      R"(storage areas 'test1' and 'test2' have the access point '/someexp' in common)",
      std::runtime_error);
}

TEST_CASE("Two storage areas cannot have an access point in common")
{
  auto constexpr conf = R"(
storage-areas:
- name: test1
  root: {0}
  access-point:
    - /ap1
    - /ap2
    - /ap3
- name: test2
  root: {0}
  access-point:
    - /ap4
    - /ap1
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(
      storm::load_configuration(is),
      R"(storage areas 'test1' and 'test2' have the access point '/ap1' in common)",
      std::runtime_error);
}

TEST_CASE("Two storage areas can have nested access points")
{
  auto constexpr conf = R"(
storage-areas:
- name: test1
  root: {}
  access-point: /someexp
- name: test2
  root: {}
  access-point: /someexp/somedir
)";
  storm::TempDirectory tmp1{};
  storm::TempDirectory tmp2{};
  std::istringstream is{fmt::format(conf, tmp1.path(), tmp2.path())};
  auto configuration = storm::load_configuration(is);
  REQUIRE_EQ(configuration.storage_areas.size(), 2);
  auto const& sa1 = configuration.storage_areas[0];
  auto const& sa2 = configuration.storage_areas[1];
  CHECK_EQ(
      sa2.access_points.front().lexically_relative(sa1.access_points.front()),
      "somedir");
}

TEST_CASE("A storage area can have nested access points")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point:
    - /someexp
    - /someexp/data
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  auto configuration = storm::load_configuration(is);
  REQUIRE_EQ(configuration.storage_areas.size(), 1);
  auto const& sa = configuration.storage_areas[0];
  REQUIRE_EQ(sa.access_points.size(), 2);
  CHECK_EQ(sa.access_points[1].lexically_relative(sa.access_points[0]), "data");
}

TEST_CASE("A storage area must have a root")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  access-point: /data
)";
  std::istringstream is{conf};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       R"(storage area 'test' has no root)",
                       std::runtime_error);
}

TEST_CASE("A storage area must have a non-empty root")
{
  std::string const conf = R"(
storage-areas:
- name: test
  root:
  access-point: /data
)";
  std::istringstream is{conf};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       R"(storage area 'test' has an empty root)",
                       std::runtime_error);
}

TEST_CASE("Two storage areas can have the same root")
{
  auto constexpr conf = R"(
storage-areas:
- name: test1
  root: {0}
  access-point: /someexp
- name: test2
  root: {0}
  access-point: /otherexp
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  auto configuration = storm::load_configuration(is);
  REQUIRE_EQ(configuration.storage_areas.size(), 2);
  auto const& sa1 = configuration.storage_areas[0];
  auto const& sa2 = configuration.storage_areas[1];
  CHECK_EQ(sa1.root, sa2.root);
}

TEST_CASE("Two storage areas can have nested roots")
{
  auto constexpr conf = R"(
storage-areas:
- name: test1
  root: {}
  access-point: /someexp
- name: test2
  root: {}
  access-point: /otherexp
)";
  storm::TempDirectory tmp{};
  auto const data = tmp.path() / "data";
  fs::create_directory(data);
  std::istringstream is{fmt::format(conf, tmp.path(), data)};
  auto configuration = storm::load_configuration(is);
  REQUIRE_EQ(configuration.storage_areas.size(), 2);
  auto const& sa1 = configuration.storage_areas[0];
  auto const& sa2 = configuration.storage_areas[1];
  CHECK_EQ(sa2.root.lexically_relative(sa1.root), "data");
}

TEST_CASE("Two storage areas can have nested access points and nested roots")
{
  auto constexpr conf = R"(
storage-areas:
- name: test1
  root: {}
  access-point: /someexp
- name: test2
  root: {}
  access-point: /someexp/somedir
)";
  storm::TempDirectory tmp{};
  auto const data = tmp.path() / "data";
  fs::create_directory(data);
  std::istringstream is{fmt::format(conf, tmp.path(), data)};
  auto configuration = storm::load_configuration(is);
  REQUIRE_EQ(configuration.storage_areas.size(), 2);
  auto const& sa1 = configuration.storage_areas[0];
  auto const& sa2 = configuration.storage_areas[1];
  CHECK_EQ(
      sa2.access_points.front().lexically_relative(sa1.access_points.front()),
      "somedir");
  CHECK_EQ(sa2.root.lexically_relative(sa1.root), "data");
}

TEST_CASE("Two storage areas can have nested access points and nested roots, "
          "inverted")
{
  auto constexpr conf = R"(
storage-areas:
- name: test1
  root: {}
  access-point: /someexp
- name: test2
  root: {}
  access-point: /someexp/somedir
)";
  storm::TempDirectory tmp{};
  auto const data = tmp.path() / "data";
  fs::create_directory(data);
  std::istringstream is{fmt::format(conf, data, tmp.path())};
  auto configuration = storm::load_configuration(is);
  REQUIRE_EQ(configuration.storage_areas.size(), 2);
  auto const& sa1 = configuration.storage_areas[0];
  auto const& sa2 = configuration.storage_areas[1];
  CHECK_EQ(
      sa2.access_points.front().lexically_relative(sa1.access_points.front()),
      "somedir");
  CHECK_EQ(sa1.root.lexically_relative(sa2.root), "data");
}

TEST_CASE("The root of a storage area must be an absolute path")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: tmp
  access-point: /someexp
)";
  std::istringstream is{conf};
  CHECK_THROWS_WITH_AS(
      storm::load_configuration(is),
      R"(root 'tmp' of storage area 'test' is not an absolute path)",
      std::runtime_error);
}

TEST_CASE("The access point of a storage area must be an absolute path")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: someexp
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(
      storm::load_configuration(is),
      R"(access point 'someexp' of storage area 'test' is not an absolute path)",
      std::runtime_error);
}

TEST_CASE("The root of a storage area must be a directory")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
)";
  storm::TempDirectory tmp{};
  auto const tmp_file = tmp.path() / "file";
  std::ofstream ofs{tmp_file};
  std::istringstream is{fmt::format(conf, tmp_file)};
  CHECK_THROWS_WITH_AS(
      storm::load_configuration(is),
      fmt::format("root '{}' of storage area 'test' is not a directory",
                  tmp_file)
          .c_str(),
      std::runtime_error);
}

TEST_CASE("The root of a storage area must have the right permissions")
{
  std::string const conf = R"(
storage-areas:
- name: test
  root: /usr/local
  access-point: /someexp
)";
  std::istringstream is{conf};
  CHECK_THROWS_WITH_AS(
      storm::load_configuration(is),
      R"(root '/usr/local' of storage area 'test' has invalid permissions)",
      std::runtime_error);
}

TEST_CASE("If the log level is not specified, it defaults to 1")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  auto config = storm::load_configuration(is);
  CHECK_EQ(config.log_level, 1);
}

TEST_CASE("If present, the log level cannot be empty")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
log-level:
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is), "log-level is null",
                       std::runtime_error);
}

TEST_CASE("The log level is an integer between 0 and 4 included")
{
  auto constexpr sa_conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
log-level: {}
)";
  storm::TempDirectory tmp{};

  for (int i : {0, 1, 2, 3, 4}) {
    std::istringstream is{fmt::format(sa_conf, tmp.path(), i)};
    auto config = storm::load_configuration(is);
    CHECK_EQ(config.log_level, i);
  }

  for (int i : {-1, 5}) {
    std::istringstream is{fmt::format(sa_conf, tmp.path(), i)};
    CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                         "invalid 'log-level' entry in configuration",
                         std::runtime_error);
  }

  {
    std::istringstream is{fmt::format(sa_conf, tmp.path(), 3.14)};
    CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                         "invalid 'log-level' entry in configuration",
                         std::runtime_error);
  }

  {
    std::istringstream is{fmt::format(sa_conf, tmp.path(), "foo")};
    CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                         "invalid 'log-level' entry in configuration",
                         std::runtime_error);
  }
}

TEST_CASE("The configuration can be loaded from a stream")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  auto configuration  = storm::load_configuration(is);
  auto& sa            = configuration.storage_areas.front();
  auto& name          = sa.name;
  auto& root          = sa.root;
  auto& access_points = sa.access_points;
  CHECK_EQ("test", name);
  CHECK_EQ(storm::PhysicalPath{tmp.path()}, root);
  CHECK_EQ(storm::LogicalPaths{"/someexp"}, access_points);
}

TEST_CASE("The configuration can be loaded from a file")
{
  auto constexpr conf = R"(storage-areas:
- name: test
  root: {}
  access-point: /someexp
)";
  storm::TempDirectory tmp{};
  auto const config_file = tmp.path() / "application.yml";
  std::ofstream ofs(config_file);
  ofs << fmt::format(conf, tmp.path());
  ofs.close();
  auto configuration  = storm::load_configuration(config_file);
  auto& sa            = configuration.storage_areas.front();
  auto& name          = sa.name;
  auto& root          = sa.root;
  auto& access_points = sa.access_points;
  CHECK_EQ("test", name);
  CHECK_EQ(storm::PhysicalPath{tmp.path()}, root);
  CHECK_EQ(storm::LogicalPaths{"/someexp"}, access_points);
}

TEST_CASE("The configuration file must have the right permissions")
{
  auto constexpr conf = R"(storage-areas:
- name: test
  root: {}
  access-point: /someexp
)";
  storm::TempDirectory tmp{};
  auto const config_file = tmp.path() / "application.yml";
  std::ofstream ofs{config_file};
  ofs << fmt::format(conf, tmp.path());
  ofs.close();
  fs::permissions(config_file, fs::perms::none);
  CHECK_THROWS_WITH_AS(
      storm::load_configuration(config_file),
      fmt::format("cannot open configuration file '{}'", config_file).c_str(),
      std::runtime_error);
}

TEST_CASE("Mirror mode can have a boolean value")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
mirror-mode: {}
)";
  storm::TempDirectory tmp{};

  for (auto s : {"true", "on", "yes"}) {
    std::istringstream is{fmt::format(conf, tmp.path(), s)};
    auto config = storm::load_configuration(is);
    CHECK_EQ(config.mirror_mode, true);
  }

  for (auto s : {"false", "off", "no"}) {
    std::istringstream is{fmt::format(conf, tmp.path(), s)};
    auto config = storm::load_configuration(is);
    CHECK_EQ(config.mirror_mode, false);
  }
}

TEST_CASE("Mirror mode cannot have an integer value")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
mirror-mode: {}
)";
  storm::TempDirectory tmp{};

  for (int i : {0, 1}) {
    std::istringstream is{fmt::format(conf, tmp.path(), i)};
    CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                         "invalid 'mirror-mode' entry in configuration",
                         std::runtime_error);
  }
}

TEST_CASE("If mirror mode is explicitly disabled, there is a write check on "
          "the SA root")
{
  std::string const conf = R"(
storage-areas:
- name: test
  root: /usr/local
  access-point: /someexp
mirror-mode: false
)";
  std::istringstream is{conf};
  CHECK_THROWS_WITH_AS(
      storm::load_configuration(is),
      R"(root '/usr/local' of storage area 'test' has invalid permissions)",
      std::runtime_error);
}

TEST_CASE("If mirror mode is enabled, there is no write check on the SA root")
{
  std::string const conf = R"(
storage-areas:
- name: test
  root: /usr/local
  access-point: /someexp
mirror-mode: on
)";
  std::istringstream is{conf};
  auto config = storm::load_configuration(is);
  CHECK_EQ(config.mirror_mode, true);
}

TEST_CASE("The default port must be 8080, if not specified")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  auto config = storm::load_configuration(is);
  CHECK_EQ(config.port, 8080);
}

TEST_CASE("The port entry cannot be empty")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port:
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is), "port is null",
                       std::runtime_error);
}

TEST_CASE("An integer port between 1 and 65535 must succeed")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 1234
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  auto config = storm::load_configuration(is);
  CHECK_EQ(config.port, 1234);
}

TEST_CASE("The port cannot be equal to '0'")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 0
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "invalid 'port' entry in configuration",
                       std::runtime_error);
}

TEST_CASE("The port cannot be negative")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: -1
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "invalid 'port' entry in configuration",
                       std::runtime_error);
}

TEST_CASE("The port cannot be larger than 65535")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 65538
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "invalid 'port' entry in configuration",
                       std::runtime_error);
}

TEST_CASE("The port cannot be a floating point number")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 3.14
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "invalid 'port' entry in configuration",
                       std::runtime_error);
}

TEST_CASE("The port can be a string, if the numeric range is valid")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: "8080"
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  auto config = storm::load_configuration(is);
  CHECK_EQ(config.port, 8080);
}

TEST_CASE("The port cannot be a string representing an not valid integer")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: foo
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "invalid 'port' entry in configuration",
                       std::runtime_error);
}

TEST_CASE("The port cannot be a string representing a floating point number")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: "3.14"
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "invalid 'port' entry in configuration",
                       std::runtime_error);
}

TEST_CASE("Accept an empty 'telemetry:'")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
telemetry:
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_NOTHROW(storm::load_configuration(is));
}

TEST_CASE("Telemetry service-name defaults to 'storm-tape'")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: https://example.org
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  auto const config = storm::load_configuration(is);
  CHECK_EQ(config.telemetry.value().service_name, "storm-tape");
}

TEST_CASE("Telemetry service-name must be not null")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  service-name:
  tracing-endpoint: https://example.org
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is), "'service-name' is null",
                       std::runtime_error);
}

TEST_CASE("Telemetry service-name cannot be an array")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  service-name: [storm-tape-example]
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "'service-name' is not a valid string",
                       std::runtime_error);
}

TEST_CASE("Telemetry has valid service-name")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  service-name: storm-tape-example
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  auto const config = storm::load_configuration(is);
  CHECK_EQ(config.telemetry.value().service_name, "storm-tape-example");
}

TEST_CASE("Tracing endpoint cannot be empty")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint:
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "'tracing-endpoint' is null", std::runtime_error);
}

TEST_CASE("Tracing endpoint w/scheme is valid")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: "http://example"
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  auto config      = storm::load_configuration(is);
  auto otel_config = config.telemetry.value();
  CHECK_EQ(otel_config.tracing_endpoint, "http://example");
}

TEST_CASE("Tracing endpoint w/scheme and top level domain is valid")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: "http://example.org"
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  auto config      = storm::load_configuration(is);
  auto otel_config = config.telemetry.value();
  CHECK_EQ(otel_config.tracing_endpoint, "http://example.org");
}

TEST_CASE("Tracing endpoint w/scheme and top level domain is valid")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: "http://example:1234"
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  auto config      = storm::load_configuration(is);
  auto otel_config = config.telemetry.value();
  CHECK_EQ(otel_config.tracing_endpoint, "http://example:1234");
}

TEST_CASE("Tracing endpoint w/scheme, top level domain and port is valid")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: "http://example.org:1234"
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  auto config      = storm::load_configuration(is);
  auto otel_config = config.telemetry.value();
  CHECK_EQ(otel_config.tracing_endpoint, "http://example.org:1234");
}

TEST_CASE("Tracing endpoint w/scheme, top level domain and path is valid")
{
  {
    auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: "http://example.org/path"
)";
    storm::TempDirectory tmp{};
    std::istringstream is{fmt::format(conf, tmp.path())};
    auto config      = storm::load_configuration(is);
    auto otel_config = config.telemetry.value();
    CHECK_EQ(otel_config.tracing_endpoint, "http://example.org/path");
  }
}

TEST_CASE("Tracing endpoint w/scheme, top level domain, port and path is valid")
{
  {
    auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: "http://example.org:1234/path"
)";
    storm::TempDirectory tmp{};
    std::istringstream is{fmt::format(conf, tmp.path())};
    auto config      = storm::load_configuration(is);
    auto otel_config = config.telemetry.value();
    CHECK_EQ(otel_config.tracing_endpoint, "http://example.org:1234/path");
  }
}

TEST_CASE("Tracing endpoint with one missing quote is not valid")
{
  {
    auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: "http://example.org
)";
    storm::TempDirectory tmp{};
    std::istringstream is{fmt::format(conf, tmp.path())};
    CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                         "'tracing-endpoint' is not a valid uri",
                         std::runtime_error);
  }
  {
    auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: http://example.org"
)";
    storm::TempDirectory tmp{};
    std::istringstream is{fmt::format(conf, tmp.path())};
    CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                         "'tracing-endpoint' is not a valid uri",
                         std::runtime_error);
  }
}

TEST_CASE("Tracing endpoint without a schema is not a valid uri")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: example.org
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "'tracing-endpoint' is not a valid uri",
                       std::runtime_error);
}

TEST_CASE("Tracing endpoint must have a valid hostname or address")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: http:://example.org:1234
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "'tracing-endpoint' has no hostname or address",
                       std::runtime_error);
}

TEST_CASE("Tracing endpoint must have a valid scheme")
{
  {
    auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: "http://example.org"
)";
    storm::TempDirectory tmp{};
    std::istringstream is{fmt::format(conf, tmp.path())};
    auto config      = storm::load_configuration(is);
    auto otel_config = config.telemetry.value();
    CHECK_EQ(otel_config.tracing_endpoint, "http://example.org");
  }

  {
    auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: "file://example.txt"
)";
    storm::TempDirectory tmp{};
    std::istringstream is{fmt::format(conf, tmp.path())};
    auto config      = storm::load_configuration(is);
    auto otel_config = config.telemetry.value();
    CHECK_EQ(otel_config.tracing_endpoint, "file://example.txt");
  }
  {
    auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: "https://example.org"
)";
    storm::TempDirectory tmp{};
    std::istringstream is{fmt::format(conf, tmp.path())};
    auto config      = storm::load_configuration(is);
    auto otel_config = config.telemetry.value();
    CHECK_EQ(otel_config.tracing_endpoint, "https://example.org");
  }
  {
    auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: ftp://example.org
)";
    storm::TempDirectory tmp{};
    std::istringstream is{fmt::format(conf, tmp.path())};
    CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                         "'tracing-endpoint' uri scheme 'ftp' is not valid",
                         std::runtime_error);
  }
  {
    auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: foo://example.org
)";
    storm::TempDirectory tmp{};
    std::istringstream is{fmt::format(conf, tmp.path())};
    CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                         "'tracing-endpoint' uri scheme 'foo' is not valid",
                         std::runtime_error);
  }
}

TEST_CASE("Tracing endpoint must have a valid hostname or address")
{
  {
    auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: http://:1234
)";
    storm::TempDirectory tmp{};
    std::istringstream is{fmt::format(conf, tmp.path())};
    CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                         "'tracing-endpoint' has no hostname or address",
                         std::runtime_error);
  }
  {
    auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: http:://example.org
)";
    storm::TempDirectory tmp{};
    std::istringstream is{fmt::format(conf, tmp.path())};
    CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                         "'tracing-endpoint' has no hostname or address",
                         std::runtime_error);
  }
}

TEST_CASE("Malformed tracing endpoint must throw")
{
  {
    auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: http://example.org::1234
)";
    storm::TempDirectory tmp{};
    std::istringstream is{fmt::format(conf, tmp.path())};
    CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                         "'tracing-endpoint' is not a valid uri",
                         std::runtime_error);
  }
  {
    auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: http://example.org::1234
)";
    storm::TempDirectory tmp{};
    std::istringstream is{fmt::format(conf, tmp.path())};
    CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                         "'tracing-endpoint' is not a valid uri",
                         std::runtime_error);
  }
}

TEST_CASE("Tracing endpoint can be an absolute file path")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: "file:///tmp/example.txt"
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  auto config      = storm::load_configuration(is);
  auto otel_config = config.telemetry.value();
  CHECK_EQ(otel_config.tracing_endpoint, "file:///tmp/example.txt");
}

TEST_CASE("Tracing endpoint can be a relative file path")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: {}
  access-point: /someexp
port: 8080
telemetry:
  tracing-endpoint: "file://example.txt"
)";
  storm::TempDirectory tmp{};
  std::istringstream is{fmt::format(conf, tmp.path())};
  auto config      = storm::load_configuration(is);
  auto otel_config = config.telemetry.value();
  CHECK_EQ(otel_config.tracing_endpoint, "file://example.txt");
}

TEST_SUITE_END;
