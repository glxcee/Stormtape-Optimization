// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#include "io.hpp"
#include <crow/query_string.h>
#include <doctest/doctest.h>

TEST_SUITE_BEGIN("IO");

TEST_CASE("An InProgressRequest accepts an `n > 0` and a `precise >= 0`, both "
          "optional")
{
  {
    auto r = storm::from_query_params(crow::query_string{"/?n=10&precise=1"},
                                      storm::InProgressRequest::tag);
    CHECK_EQ(r.n_files, 10);
    CHECK_EQ(r.precise, 1);
  }
  {
    auto r = storm::from_query_params(crow::query_string{"/"},
                                      storm::InProgressRequest::tag);
    CHECK_EQ(r.n_files, 1'000);
    CHECK_EQ(r.precise, 0);
  }
  {
    auto r = storm::from_query_params(crow::query_string{"/?n=10"},
                                      storm::InProgressRequest::tag);
    CHECK_EQ(r.n_files, 10);
    CHECK_EQ(r.precise, 0);
  }
  {
    auto r = storm::from_query_params(crow::query_string{"/?precise=1"},
                                      storm::InProgressRequest::tag);
    CHECK_EQ(r.n_files, 1'000);
    CHECK_EQ(r.precise, 1);
  }
  {
    auto r = storm::from_query_params(crow::query_string{"/?precise=2"},
                                      storm::InProgressRequest::tag);
    CHECK_EQ(r.n_files, 1'000);
    CHECK_EQ(r.precise, 2);
  }
  {
    auto r = storm::from_query_params(crow::query_string{"/?n=0"},
                                      storm::InProgressRequest::tag);
    CHECK_EQ(r.n_files, 1'000);
    CHECK_EQ(r.precise, 0);
  }
  {
    auto r = storm::from_query_params(crow::query_string{"/?n=-1"},
                                      storm::InProgressRequest::tag);
    CHECK_EQ(r.n_files, 1'000);
    CHECK_EQ(r.precise, 0);
  }
  {
    auto r = storm::from_query_params(crow::query_string{"/?precise=-1"},
                                      storm::InProgressRequest::tag);
    CHECK_EQ(r.n_files, 1'000);
    CHECK_EQ(r.precise, 0);
  }
}

TEST_CASE("TakeOverRequest accepts 'n_files' between 1 and 1'000'000, included")
{
  {
    auto n_files = storm::from_body_params("", storm::TakeOverRequest::tag);
    CHECK_EQ(n_files, 1);
  }
  {
    auto n_files =
        storm::from_body_params("first=10", storm::TakeOverRequest::tag);
    CHECK_EQ(n_files, 10);
  }
  {
    CHECK_THROWS_WITH_AS(
        storm::from_body_params("first=0", storm::TakeOverRequest::tag),
        "Invalid number of files", storm::BadRequest);
  }
  {
    CHECK_THROWS_WITH_AS(
        storm::from_body_params("first=-1", storm::TakeOverRequest::tag),
        "Invalid number of files", storm::BadRequest);
  }
  {
    CHECK_THROWS_WITH_AS(
        storm::from_body_params("first=1000001", storm::TakeOverRequest::tag),
        "Invalid number of files", storm::BadRequest);
  }
  {
    CHECK_THROWS_WITH_AS(
        storm::from_body_params("first=3.14", storm::TakeOverRequest::tag),
        "Invalid number of files", storm::BadRequest);
  }
  {
    CHECK_THROWS_WITH_AS(
        storm::from_body_params("foo=10", storm::TakeOverRequest::tag),
        "Invalid body content", storm::BadRequest);
  }
}

TEST_SUITE_END;
