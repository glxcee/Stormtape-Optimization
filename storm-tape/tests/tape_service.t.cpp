// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#include "archiveinfo_response.hpp"
#include "cancel_response.hpp"
#include "extended_attributes.hpp"
#include "file.hpp"
#include "fixture.t.hpp"
#include "in_progress_request.hpp"
#include "in_progress_response.hpp"
#include "readytakeover_response.hpp"
#include "requests_with_paths.hpp"
#include "stage_request.hpp"
#include "stage_response.hpp"
#include "status_response.hpp"
#include "takeover_request.hpp"
#include "takeover_response.hpp"
#include "types.hpp"

#include <doctest/doctest.h>
#include <algorithm>
#include <cstdlib>
#include <iostream>

namespace fs = std::filesystem;

auto archive_info = [](storm::TapeService& service, storm::Files const& files) {
  struct PathInfoVisitor
  {
    std::string operator()(storm::Locality locality) const
    {
      return to_string(locality);
    }

    std::string operator()(std::string const& msg) const
    {
      return msg;
    }
  };

  storm::LogicalPaths paths;
  paths.reserve(files.size());
  std::transform(files.begin(), files.end(), std::back_inserter(paths),
                 [](auto const& f) { return f.logical_path; });
  storm::ArchiveInfoRequest request{paths};

  storm::ArchiveInfoResponse response = service.archive_info(request);
  auto& infos                         = response.infos;
  CHECK_EQ(infos.size(), 2);
  std::vector<std::string> localities;
  localities.reserve(infos.size());
  std::transform(infos.begin(), infos.end(), std::back_inserter(localities),
                 [](storm::PathInfo const& info) {
                   PathInfoVisitor visitor{};
                   return boost::variant2::visit(visitor, info.info);
                 });
  return localities;
};

namespace {

storm::File& find(storm::Files& files, storm::PhysicalPath const& path)
{
  auto it = std::find_if(files.begin(), files.end(),
                         [&](auto& f) { return f.physical_path == path; });
  REQUIRE_NE(it, files.end());
  return *it;
}

} // namespace
TEST_SUITE_BEGIN("TapeService");

// Add coverage for hpp files. The usage of 'auto' produces 0% coverage on these
// header files
TEST_CASE("Increase Coverage")
{
  soci::session sql;
  storm::Configuration config;
  storm::LocalStorage storage;
}

TEST_CASE("Simple Stage")
{
  auto fixture      = storm::TestFixture();
  auto& service     = fixture.get_service();
  auto const& files = fixture.get_files();
  auto const now    = std::time(nullptr);
  storm::StageRequest request{files, now, 0, 0};
  REQUIRE_GE(request.files.size(), 2);
  fixture.create_stub_on_disk_at(0);
  fixture.create_file_on_disk_at(1);

  // Do stage
  storm::StageResponse stage_response = service.stage(std::move(request));
  storm::StageId id                   = stage_response.id();

  {
    auto status_response = service.status(id);
    auto& stage          = status_response.stage();
    CHECK(stage.files[0].state == storm::File::State::submitted);
    CHECK(stage.files[1].state == storm::File::State::completed);
  }
  {
    auto localities = archive_info(service, files);
    CHECK_EQ(localities[0], "TAPE");
    CHECK_EQ(localities[1], "DISK_AND_TAPE");
  }
  {
    fs::remove(files[0].physical_path);
    fs::remove(files[1].physical_path);

    auto status_response = service.status(id);
    auto& stage          = status_response.stage();

    CHECK(stage.files[0].state == storm::File::State::failed);
    CHECK(stage.files[1].state == storm::File::State::completed);

    auto localities = archive_info(service, files);
    CHECK_EQ(localities[0], "No such file or directory");
    CHECK_EQ(localities[1], "No such file or directory");
  }
}

TEST_CASE("In Progress")
{
  auto fixture      = storm::TestFixture();
  auto& service     = fixture.get_service();
  auto const& files = fixture.get_files();
  auto const now    = std::time(nullptr);
  storm::StageRequest request{files, now, 0, 0};
  REQUIRE_GE(request.files.size(), 2);
  fixture.create_stub_on_disk_at(0);
  fixture.create_stub_on_disk_at(1);

  // Do stage
  auto stage_response = service.stage(std::move(request));
  auto id             = stage_response.id();

  {
    auto localities = archive_info(service, files);
    CHECK_EQ(localities[0], "TAPE");
    CHECK_EQ(localities[1], "TAPE");
  }
  {
    storm::InProgressResponse const resp = service.in_progress({.precise = 0});
    CHECK(resp.paths.empty());
  }
  {
    auto const resp = service.in_progress({.precise = 1});
    CHECK(resp.paths.empty());
  }
  {
    auto const to_resp = service.take_over({.n_files = 10});

    CHECK_EQ(to_resp.paths.size(), 2);
    {
      auto const resp = service.in_progress({.precise = 0});
      CHECK_EQ(resp.paths.size(), 2);
    }
    {
      auto const resp = service.in_progress({.precise = 1});
      CHECK_EQ(resp.paths.size(), 2);
    }
    {
      auto localities = archive_info(service, files);
      CHECK_EQ(localities[0], "TAPE");
      CHECK_EQ(localities[1], "TAPE");
    }
    {
      // Simulate the end of the recall
      fixture.create_file_on_disk_at(0);
      storm::remove_xattr(files[0].physical_path,
                          storm::XAttrName{"user.TSMRecT"});
    }
    {
      auto const resp = service.in_progress({.precise = 0});
      CHECK_EQ(resp.paths.size(), 2);
    }
    {
      auto const resp = service.in_progress({.precise = 1});
      CHECK_EQ(resp.paths.size(), 1);
    }
    {
      auto localities = archive_info(service, files);
      CHECK_EQ(localities[0], "DISK_AND_TAPE");
      CHECK_EQ(localities[1], "TAPE");
    }

    auto status_response = service.status(id);
    auto& files_status   = status_response.stage().files;

    CHECK_EQ(files_status.size(), 2);
    CHECK(std::all_of(files_status.begin(), files_status.end(),
                      [&files](auto const& f) {
                        if (f.physical_path == files[0].physical_path) {
                          return f.state == storm::File::State::completed;
                        } else if (f.physical_path == files[1].physical_path) {
                          return f.state == storm::File::State::started;
                        } else {
                          return false;
                        }
                      }));
  }
  {
    auto const resp = service.in_progress({.precise = 0});
    CHECK_EQ(resp.paths.size(), 1);
  }
  {
    auto const resp = service.in_progress({.precise = 1});
    CHECK_EQ(resp.paths.size(), 1);
  }
}

TEST_CASE("Stage w/ Recall")
{
  auto fixture      = storm::TestFixture();
  auto& service     = fixture.get_service();
  auto const& files = fixture.get_files();
  auto const now    = std::time(nullptr);
  storm::StageRequest request{files, now - 2, 0, 0};

  REQUIRE(request.files.size() == files.size());
  constexpr std::size_t index_initially_stub{0};
  constexpr std::size_t index_initially_on_disk{1};
  auto const path_initially_stub =
      fixture.create_stub_on_disk_at(index_initially_stub);
  auto const path_initially_on_disk =
      fixture.create_file_on_disk_at(index_initially_on_disk);

  auto id = service.stage(std::move(request)).id();

  {
    auto maybe_stage = fixture.get_db().find(id);
    CHECK(maybe_stage.has_value());
    auto& stage = maybe_stage.value();
    CHECK(
        std::all_of(stage.files.begin(), stage.files.end(), [](auto const& f) {
          return f.state == storm::File::State::submitted;
        }));
    CHECK(
        std::all_of(stage.files.begin(), stage.files.end(), [](auto const& f) {
          return f.started_at == f.finished_at && f.started_at == 0;
        }));
  }
  {
    // the stage doesn't check if the files are already on disk, so all
    // the files as potentially recallable
    auto resp = service.ready_take_over();
    CHECK_EQ(resp.n_ready, files.size());
  }
  {
    // status correctly mirrors the situation on the file system
    auto resp   = service.status(id);
    auto& stage = resp.stage();
    REQUIRE(stage.files.size() == files.size());
    auto& file_initially_stub    = find(stage.files, path_initially_stub);
    auto& file_initially_on_disk = find(stage.files, path_initially_on_disk);

    CHECK(file_initially_stub.state == storm::File::State::submitted);
    CHECK(file_initially_stub.started_at == 0);
    CHECK(file_initially_stub.finished_at == 0);

    CHECK(file_initially_on_disk.state == storm::File::State::completed);
    CHECK(file_initially_on_disk.started_at >= now);
    CHECK(file_initially_on_disk.finished_at
          == file_initially_on_disk.started_at);

    CHECK(stage.created_at == now - 2);
    CHECK(stage.started_at == file_initially_on_disk.started_at);
    CHECK(stage.completed_at == 0);
  }
  {
    // the DB has been correctly updated
    auto maybe_stage = fixture.get_db().find(id);
    REQUIRE(maybe_stage.has_value());
    auto& stage = maybe_stage.value();
    REQUIRE(stage.files.size() == files.size());

    auto& file_initially_stub    = find(stage.files, path_initially_stub);
    auto& file_initially_on_disk = find(stage.files, path_initially_on_disk);

    CHECK(file_initially_stub.state == storm::File::State::submitted);
    CHECK(file_initially_stub.started_at == 0);
    CHECK(file_initially_stub.finished_at == 0);

    CHECK(file_initially_on_disk.state == storm::File::State::completed);
    CHECK(file_initially_on_disk.started_at >= now);
    CHECK(file_initially_on_disk.finished_at
          == file_initially_on_disk.started_at);

    CHECK(stage.created_at == now - 2);
    CHECK(stage.started_at == file_initially_on_disk.started_at);
    CHECK(stage.completed_at == 0);
  }
  {
    // no file is in progress
    auto resp = service.in_progress();
    CHECK(resp.paths.empty());
  }
  {
    // only one file is now potentially recallable
    auto resp = service.ready_take_over();
    CHECK(resp.n_ready == 1);
  }
  {
    // only one file is indeed recallable
    auto resp = service.take_over({42});
    CHECK(resp.paths.size() == 1);
    CHECK(resp.paths.front() == path_initially_stub);
    storm::XAttrName const tsmrect{"user.TSMRecT"};
    CHECK(has_xattr(path_initially_stub, tsmrect));
    CHECK_FALSE(has_xattr(path_initially_on_disk, tsmrect));
  }
  { // one file in progress, one file completed
    auto resp   = service.status(id);
    auto& stage = resp.stage();
    REQUIRE(resp.id() == id);
    REQUIRE(stage.files.size() == files.size());

    auto& file_initially_stub    = find(stage.files, path_initially_stub);
    auto& file_initially_on_disk = find(stage.files, path_initially_on_disk);

    CHECK(file_initially_stub.state == storm::File::State::started);
    CHECK(file_initially_stub.started_at >= now);
    CHECK(file_initially_stub.finished_at == 0);

    CHECK(file_initially_on_disk.state == storm::File::State::completed);
    CHECK(file_initially_on_disk.started_at >= now);
    CHECK(file_initially_on_disk.finished_at
          == file_initially_on_disk.started_at);
  }
  { // the DB is correctly updated
    auto maybe_stage = fixture.get_db().find(id);
    REQUIRE(maybe_stage.has_value());
    auto& stage = maybe_stage.value();
    REQUIRE(stage.files.size() == 2);

    auto& file_initially_stub    = find(stage.files, path_initially_stub);
    auto& file_initially_on_disk = find(stage.files, path_initially_on_disk);

    CHECK(file_initially_stub.state == storm::File::State::started);
    CHECK(file_initially_stub.started_at >= now);
    CHECK(file_initially_stub.finished_at == 0);

    CHECK(file_initially_on_disk.state == storm::File::State::completed);
    CHECK(file_initially_on_disk.started_at >= now);
    CHECK(file_initially_on_disk.finished_at
          == file_initially_on_disk.started_at);
  }
  {
    auto resp = service.in_progress();
    CHECK(resp.paths.size() == 1);
    CHECK(resp.paths.front() == path_initially_stub);
  }
  {
    // simulate the end of the recall
    fixture.create_file_on_disk_at(index_initially_stub);
    remove_xattr(path_initially_stub, storm::XAttrName{"user.TSMRecT"});
  }
  {
    auto resp   = service.status(id);
    auto& stage = resp.stage();
    REQUIRE(resp.id() == id);
    REQUIRE(stage.files.size() == files.size());

    auto& file_initially_stub    = find(stage.files, path_initially_stub);
    auto& file_initially_on_disk = find(stage.files, path_initially_on_disk);

    CHECK(file_initially_stub.state == storm::File::State::completed);
    CHECK(file_initially_stub.started_at >= file_initially_on_disk.started_at);
    CHECK(file_initially_stub.finished_at >= file_initially_stub.started_at);
    CHECK(file_initially_on_disk.state == storm::File::State::completed);
    CHECK(file_initially_on_disk.started_at >= now);
    CHECK(file_initially_on_disk.finished_at == file_initially_on_disk.started_at);

    CHECK(stage.created_at == now - 2);
    CHECK(stage.started_at == file_initially_on_disk.started_at);
    CHECK(stage.completed_at == file_initially_stub.finished_at);
    CHECK(stage.completed_at >= stage.started_at);
  }
  {
    // check from db
    auto maybe_stage = fixture.get_db().find(id);
    REQUIRE(maybe_stage.has_value());
    auto& stage = maybe_stage.value();
    REQUIRE(stage.files.size() == files.size());

    auto& file_initially_stub    = find(stage.files, path_initially_stub);
    auto& file_initially_on_disk = find(stage.files, path_initially_on_disk);

    CHECK(stage.created_at == now - 2);
    CHECK(stage.started_at == file_initially_on_disk.started_at);
    CHECK(stage.completed_at == file_initially_stub.finished_at);
    CHECK(stage.completed_at >= stage.started_at);
  }
  {
    // Check in progress paths
    auto resp = service.in_progress();
    CHECK(resp.paths.empty());
  }
}

TEST_CASE("Cancel")
{
  auto fixture      = storm::TestFixture();
  auto& service     = fixture.get_service();
  auto const& files = fixture.get_files();
  auto const now    = std::time(nullptr);
  storm::StageRequest request{files, now, 0, 0};
  REQUIRE_GE(request.files.size(), 2);
  fixture.create_stub_on_disk_at(0);
  fixture.create_file_on_disk_at(1);
  // Do stage
  auto stage_response = service.stage(std::move(request));
  auto& id            = stage_response.id();

  // Status (update db)
  service.status(id);

  // Check in progress paths after recall
  auto maybe_stage = fixture.get_db().find(id);
  CHECK(maybe_stage.has_value());
  auto& stage = maybe_stage.value();
  CHECK_EQ(stage.files.size(), 2);

  for (auto&& staged_file : stage.files) {
    if (staged_file.physical_path == files[0].physical_path) {
      CHECK_EQ(staged_file.state, storm::File::State::submitted);
    } else if (staged_file.physical_path == files[1].physical_path) {
      CHECK_EQ(staged_file.state, storm::File::State::completed);
    } else {
      CHECK_FALSE(true);
    }
  }

  auto to_response = service.take_over({42});
  CHECK_EQ(to_response.paths.size(), 1);

  // Check in progress
  storm::InProgressResponse in_progress = service.in_progress();
  CHECK_EQ(in_progress.paths.size(), 1);
  CHECK_EQ(in_progress.paths[0], files[0].physical_path);

  // Do cancel
  storm::LogicalPaths paths;
  std::transform(
      files.begin(), files.end(), std::back_inserter(paths),
      [](storm::File const& f) { return storm::LogicalPath{f.logical_path}; });

  service.cancel(id, storm::CancelRequest{paths});
  // Do status
  stage = service.status(id).stage();
  CHECK_EQ(stage.created_at, now);
  CHECK_GE(stage.started_at, now);
  CHECK_EQ(stage.started_at, stage.completed_at);
  CHECK(std::all_of(stage.files.begin(), stage.files.end(), [](auto& f) {
    return f.state == storm::File::State::cancelled;
  }));
  // Check in progress paths
  in_progress = service.in_progress();
  CHECK(in_progress.paths.empty());
}

TEST_CASE("Empty Stage")
{
  auto fixture        = storm::TestFixture();
  auto& service       = fixture.get_service();
  auto const now      = std::time(nullptr);
  auto const response = service.stage({{}, now, 0, 0});
  REQUIRE_EQ(response.files().size(), 0);
  auto const status = service.status(response.id());
}
