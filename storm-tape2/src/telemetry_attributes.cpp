// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#include "telemetry_attributes.hpp"

#include <opentelemetry/semconv/client_attributes.h>
#include <opentelemetry/semconv/incubating/http_attributes.h>

#include <utility>

namespace storm {

OtelAttributes to_semconv(crow::request const& req)
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

OtelAttributes to_semconv(crow::request const& req, std::string_view operation)
{
  auto attrs{to_semconv(req)};
  attrs.insert({"storm.operation", std::string{operation}});
  return attrs;
}

} // namespace storm
