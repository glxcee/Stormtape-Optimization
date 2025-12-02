// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#ifndef STORM_IO_HPP
#define STORM_IO_HPP

#include "errors.hpp"
#include "file.hpp"
#include "in_progress_request.hpp"
#include "requests_with_paths.hpp"
#include "stage_request.hpp"
#include "takeover_request.hpp"
#include <boost/json.hpp>
#include <string_view>

namespace crow {
class request;
class response;
class query_string;
} // namespace crow

namespace storm {

class StageResponse;
class StatusResponse;
class CancelResponse;
class DeleteResponse;
class ReleaseResponse;
class ArchiveInfoResponse;
class ReadyTakeOverResponse;
class TakeOverResponse;
class InProgressResponse;
class Configuration;

crow::response to_crow_response(StageResponse const& resp);

crow::response to_crow_response(StatusResponse const& resp);
crow::response to_crow_response(DeleteResponse const& resp);
crow::response to_crow_response(CancelResponse const& resp);
crow::response to_crow_response(ReleaseResponse const& resp);
boost::json::array archive_to_json(Files const& file,
                                   boost::json::array& jbody);
crow::response to_crow_response(ArchiveInfoResponse const& resp);

crow::response to_crow_response(ReadyTakeOverResponse const& resp);
crow::response to_crow_response(TakeOverResponse const& resp);
crow::response to_crow_response(InProgressResponse const& resp);
crow::response to_crow_response(storm::HttpError const& exception);

Files from_json(std::string_view body, StageRequest::Tag);
LogicalPaths from_json(std::string_view body, RequestWithPaths::Tag);

std::size_t from_body_params(std::string_view body, TakeOverRequest::Tag);
InProgressRequest from_query_params(crow::query_string const& qs,
                                    InProgressRequest::Tag);

} // namespace storm

#endif
