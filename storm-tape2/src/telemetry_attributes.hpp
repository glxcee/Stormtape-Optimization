// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#ifndef STORM_TELEMETRY_ATTRIBUTES_HPP
#define STORM_TELEMETRY_ATTRIBUTES_HPP

#include <map>
#include <string>
#include <string_view>

#include <crow/http_request.h>

namespace storm {

using OtelAttributes = std::map<std::string, std::string>;
OtelAttributes to_semconv(crow::request const& req);
OtelAttributes to_semconv(crow::request const& req, std::string_view operation);

} // namespace storm

#endif
