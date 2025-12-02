// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#include "errors.hpp"
#include "archiveinfo_response.hpp"
#include "cancel_response.hpp"
#include "delete_response.hpp"
#include "fixture.t.hpp"
#include "io.hpp"
#include "release_response.hpp"
#include "stage_response.hpp"
#include "status_response.hpp"
#include "tape_service.hpp"
#include <doctest/doctest.h>

namespace storm {

static const StageId invalid_id{"123-abcd-666"};
static StageId make_stage(TapeService& service)
{
  StageRequest req{{File{"test", "test"}}, 0, 0, 0};
  return service.stage(req).id();
}

TEST_SUITE_BEGIN("Errors");
TEST_CASE("Invalid Stage")
{
  {
    auto json =
        R"({"files":[{"path":"/tmp//example.txt"},{"path":"/tmp/example2.txt"}]})";
    CHECK_NOTHROW(from_json(json, StageRequest::tag));
  }
  {
    auto json =
        R"({"files"[{"path":"/tmp//example.txt"},{"path":"/tmp/example2.txt"}]})";
    CHECK_THROWS_AS_MESSAGE(from_json(json, StageRequest::tag), BadRequest,
                            "Invalid JSON");
  }
  {
    auto json =
        R"(({"FiLeS":[{"path":"/tmp//example.txt"},{"path":"/tmp/example2.txt"}]})";
    CHECK_THROWS_AS_MESSAGE(from_json(json, StageRequest::tag), BadRequest,
                            "Invalid JSON");
  }
}

TEST_CASE("Stage not found")
{
  auto fixture        = storm::TestFixture();
  auto& service       = fixture.get_service();
  auto const valid_id = make_stage(service);
  CHECK_THROWS_AS_MESSAGE(service.status(invalid_id), StageNotFound,
                          "Stage Not Found");
  CHECK_NOTHROW(service.status(valid_id));
}

TEST_CASE("Invalid Cancel/Stage not found")
{
  auto fixture        = storm::TestFixture();
  auto& service       = fixture.get_service();
  {
    auto json = R"({"paths":["/tmp/example.txt","/tmp/example2.txt"]})";
    CHECK_NOTHROW(from_json(json, CancelRequest::tag));
  }
  {
    auto json = R"({"files":["/tmp/example.txt","/tmp/example2.txt"]})";
    CHECK_THROWS_AS_MESSAGE(from_json(json, CancelRequest::tag), BadRequest,
                            "Invalid JSON");
  }
  {
    auto json = R"({"paths":["/tmp/example.txt""/tmp/example2.txt"]})";
    CHECK_THROWS_AS_MESSAGE(from_json(json, CancelRequest::tag), BadRequest,
                            "Invalid JSON");
  }
  auto const valid_id = make_stage(service);
  CHECK_NOTHROW(service.cancel(valid_id, CancelRequest{}));
  CHECK_THROWS_AS_MESSAGE(service.cancel(invalid_id, CancelRequest{}),
                          StageNotFound, "Stage Not Found");
}

TEST_CASE("Stage not found during Delete")
{
  auto fixture        = storm::TestFixture();
  auto& service       = fixture.get_service();
  auto const valid_id = make_stage(service);
  CHECK_THROWS_AS_MESSAGE(service.erase(invalid_id), StageNotFound,
                          "Stage Not Found");
  CHECK_NOTHROW(service.erase(valid_id));
}

TEST_CASE("Invalid Release/Stage not found")
{
  auto fixture        = storm::TestFixture();
  auto& service       = fixture.get_service();
  {
    auto json = R"({"paths":["/tmp/example.txt","/tmp/example2.txt"]})";
    CHECK_NOTHROW(from_json(json, ReleaseRequest::tag));
  }
  {
    auto json = R"({"files":["/tmp/example.txt","/tmp/example2.txt"]})";
    CHECK_THROWS_AS_MESSAGE(from_json(json, ReleaseRequest::tag), BadRequest,
                            "Invalid JSON");
  }
  {
    auto json = R"({"paths":["/tmp/example.txt""/tmp/example2.txt"]})";
    CHECK_THROWS_AS_MESSAGE(from_json(json, ReleaseRequest::tag), BadRequest,
                            "Invalid JSON");
  }
  auto const valid_id = make_stage(service);
  CHECK_THROWS_AS_MESSAGE(service.release(invalid_id, {}), StageNotFound,
                          "Stage Not Found");
  CHECK_NOTHROW(service.release(valid_id, {}));
}

TEST_CASE("Invalid ArchiveInfo")
{
  {
    auto json = R"({"paths":["/tmp/example.txt","/tmp/example2.txt"]})";
    CHECK_NOTHROW(from_json(json, ArchiveInfoRequest::tag));
  }
  {
    auto json = R"({"files":["/tmp/example.txt","/tmp/example2.txt"]})";
    CHECK_THROWS_AS_MESSAGE(from_json(json, ArchiveInfoRequest::tag),
                            BadRequest, "Invalid JSON");
  }
  {
    auto json = R"({"paths":["/tmp/example.txt""/tmp/example2.txt"]})";
    CHECK_THROWS_AS_MESSAGE(from_json(json, ArchiveInfoRequest::tag),
                            BadRequest, "Invalid JSON");
  }
}

TEST_CASE("Invalid TakeOver")
{
  {
    auto query_string = "first=123";
    CHECK_NOTHROW(from_body_params(query_string, TakeOverRequest::tag));
  }
  {
    auto query_string = "second=123";
    CHECK_THROWS_AS_MESSAGE(from_json(query_string, ArchiveInfoRequest::tag),
                            BadRequest, "Invalid query parameters");
  }
  {
    auto query_string = "first=12.3";
    CHECK_THROWS_AS_MESSAGE(from_json(query_string, ArchiveInfoRequest::tag),
                            BadRequest, "Invalid query parameters");
  }
  {
    auto query_string = "first=p123";
    CHECK_THROWS_AS_MESSAGE(from_json(query_string, ArchiveInfoRequest::tag),
                            BadRequest, "Invalid query parameters");
  }
  {
    auto query_string = "first=123x";
    CHECK_THROWS_AS_MESSAGE(from_json(query_string, ArchiveInfoRequest::tag),
                            BadRequest, "Invalid query parameters");
  }
}
} // namespace storm
