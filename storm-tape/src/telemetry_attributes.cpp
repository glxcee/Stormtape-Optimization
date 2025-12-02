// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#include "telemetry_attributes.hpp"

#include <opentelemetry/semconv/client_attributes.h>
#include <opentelemetry/semconv/incubating/http_attributes.h>

namespace storm {

OtelAttributesMap to_semconv(crow::request const& req)
{
  using namespace opentelemetry;
  // clang-format off
  return {
    {semconv::http::kHttpRoute,           req.url},
    {semconv::http::kHttpRequestMethod,   crow::method_name(req.method)},
    {semconv::http::kHttpRequestBodySize, std::to_string(req.body.size())},
    {semconv::client::kClientAddress,     req.remote_ip_address},
  };
  // clang-format on
}

} // namespace storm
