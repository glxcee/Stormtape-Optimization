// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#ifndef STORM_HTTP_TEXT_MAP_CARRIER
#define STORM_HTTP_TEXT_MAP_CARRIER

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <opentelemetry/context/propagation/text_map_propagator.h>
#include <opentelemetry/trace/propagation/http_trace_context.h>

namespace storm {

// https://github.com/open-telemetry/opentelemetry-cpp/blob/4998eb178601f69481e1629e5464d0272532ec9e/examples/http/tracer_common.h#L29
template<typename T>
class HttpTextMapCarrier
    : public opentelemetry::context::propagation::TextMapCarrier
{
  T m_headers;

 public:
  HttpTextMapCarrier(T const& headers)
      : m_headers(headers)
  {}
  HttpTextMapCarrier() = default;

  std::string_view Get(std::string_view key) const noexcept override
  {
    auto it = m_headers.find({key.data(), key.size()});
    if (it != m_headers.end()) {
      return it->second;
    }
    return "";
  }

  void Set(std::string_view key, std::string_view value) noexcept override
  {
    m_headers.insert(std::pair{std::string{key}, std::string{value}});
  }
};

} // namespace storm
#endif
