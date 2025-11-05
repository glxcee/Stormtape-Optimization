// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#ifndef STORM_TRACER_PROVIDER_HPP
#define STORM_TRACER_PROVIDER_HPP

#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/trace/tracer.h>

#include <memory>
#include <string>
#include <string_view>

namespace storm {

class TracerProvider
{
  std::shared_ptr<opentelemetry::trace::TracerProvider> m_provider;

 public:
  TracerProvider(std::string_view hostname, std::string_view service_name,
                 std::string_view otlp_collector_endpoint);
  TracerProvider(TracerProvider const&)             = delete;
  TracerProvider& operator=(TracerProvider const&)  = delete;
  TracerProvider& operator=(TracerProvider const&&) = delete;
  ~TracerProvider();

  static std::shared_ptr<opentelemetry::trace::Tracer> get_tracer();
};

} // namespace storm

#endif
