// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#ifndef STORM_TELEMETRY_ATTRIBUTES_HPP
#define STORM_TELEMETRY_ATTRIBUTES_HPP

#include <map>
#include <string>

#include <crow/http_request.h>

namespace storm {

struct OtelAttribute
{
  auto static constexpr batch_size     = "storm.operation.size";
  auto static constexpr operation_name = "storm.operation.name";
};

using OtelAttributesMap = std::map<std::string, std::string>;
OtelAttributesMap to_semconv(crow::request const& req);
} // namespace storm

#endif
