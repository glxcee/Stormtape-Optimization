// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#include "tracer_provider.hpp"
#include "io.hpp"

#include <chrono>
#include <iostream>
#include <memory>

#include <opentelemetry/context/propagation/global_propagator.h>
#include <opentelemetry/exporters/otlp/otlp_http_exporter_factory.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/sdk/trace/batch_span_processor_factory.h>
#include <opentelemetry/sdk/trace/batch_span_processor_options.h>
#include <opentelemetry/sdk/trace/samplers/always_off_factory.h>
#include <opentelemetry/sdk/trace/samplers/parent_factory.h>
#include <opentelemetry/sdk/trace/tracer_context_factory.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/semconv/incubating/host_attributes.h>
#include <opentelemetry/semconv/service_attributes.h>
#include <opentelemetry/trace/propagation/http_trace_context.h>
#include <opentelemetry/trace/provider.h>

namespace storm {

TracerProvider::TracerProvider(std::string_view hostname,
                               std::string_view service_name,
                               std::string_view otlp_collector_endpoint)
{
  using namespace std::chrono_literals;
  using namespace opentelemetry::sdk::trace;
  using namespace opentelemetry::sdk::resource;
  using namespace opentelemetry::exporter::otlp;
  using namespace opentelemetry::trace;

  // initialize exporter(s) and tracer provider
  OtlpHttpExporterOptions exporter_opts{};
  exporter_opts.url                       = otlp_collector_endpoint;
  exporter_opts.retry_policy_max_attempts = 1;
  exporter_opts.timeout                   = 1000ms;
  auto exporter = OtlpHttpExporterFactory::Create(exporter_opts);

  BatchSpanProcessorOptions proc_opts{};
  proc_opts.max_queue_size        = 1024;
  proc_opts.max_export_batch_size = 512;
  proc_opts.schedule_delay_millis = 5000ms;
  auto processor =
      BatchSpanProcessorFactory::Create(std::move(exporter), proc_opts);

  std::vector<std::unique_ptr<SpanProcessor>> processors{};
  processors.push_back(std::move(processor));

  using namespace opentelemetry::semconv;
  // clang-format off
  ResourceAttributes attributes{{
    {service::kServiceName, std::string{service_name}},
    {host::kHostName, std::string{hostname}}
  }};
  // clang-format on

  auto resource         = Resource::Create(attributes);
  auto fallback_sampler = AlwaysOffSamplerFactory::Create();
  auto sampler = ParentBasedSamplerFactory::Create(std::move(fallback_sampler));
  auto context = TracerContextFactory::Create(std::move(processors), resource,
                                              std::move(sampler));

  m_provider = TracerProviderFactory::Create(std::move(context));
  Provider::SetTracerProvider(m_provider);

  using namespace opentelemetry::context::propagation;
  using namespace opentelemetry::trace::propagation;
  GlobalTextMapPropagator::SetGlobalPropagator(
      std::shared_ptr<TextMapPropagator>(new HttpTraceContext()));
}

TracerProvider::~TracerProvider()
{
  opentelemetry::trace::Provider::SetTracerProvider({});
}

std::shared_ptr<opentelemetry::trace::Tracer> TracerProvider::get_tracer()
{
  if (auto provider = opentelemetry::trace::Provider::GetTracerProvider()) {
    return provider->GetTracer("storm-tracer");
  }
  return {};
}

} // namespace storm
