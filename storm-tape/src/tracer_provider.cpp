// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#include "tracer_provider.hpp"

#include <chrono>
#include <memory>
#include <utility>

#include <fmt/core.h>

#include <fmt/format.h>
#include <opentelemetry/context/propagation/global_propagator.h>
#include <opentelemetry/exporters/ostream/span_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_http_exporter_factory.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/sdk/trace/batch_span_processor_factory.h>
#include <opentelemetry/sdk/trace/batch_span_processor_options.h>
#include <opentelemetry/sdk/trace/exporter.h>
#include <opentelemetry/sdk/trace/samplers/always_off_factory.h>
#include <opentelemetry/sdk/trace/samplers/always_on_factory.h>
#include <opentelemetry/sdk/trace/samplers/parent_factory.h>
#include <opentelemetry/sdk/trace/simple_processor_factory.h>
#include <opentelemetry/sdk/trace/tracer_context_factory.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/semconv/incubating/host_attributes.h>
#include <opentelemetry/semconv/service_attributes.h>
#include <opentelemetry/trace/propagation/http_trace_context.h>
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/span.h>

namespace storm {

namespace {

auto create_otlp_context(opentelemetry::sdk::resource::Resource& resource,
                         std::string_view otlp_collector_endpoint)
{
  using namespace std::chrono_literals;
  namespace sdk_trace = opentelemetry::sdk::trace;
  namespace otlp      = opentelemetry::exporter::otlp;

  //  otlp exporter
  otlp::OtlpHttpExporterOptions exporter_opts{};
  exporter_opts.url                       = otlp_collector_endpoint;
  exporter_opts.retry_policy_max_attempts = 1;
  exporter_opts.timeout                   = 1000ms;
  auto exporter = otlp::OtlpHttpExporterFactory::Create(exporter_opts);

  //  batch span processor
  sdk_trace::BatchSpanProcessorOptions proc_opts{};
  proc_opts.max_queue_size        = 1024;
  proc_opts.max_export_batch_size = 512;
  proc_opts.schedule_delay_millis = 5000ms;
  auto processor = sdk_trace::BatchSpanProcessorFactory::Create(
      std::move(exporter), proc_opts);
  std::vector<std::unique_ptr<sdk_trace::SpanProcessor>> processors{};
  processors.push_back(std::move(processor));

  // parent based sampler (fallback always off)
  auto fallback_sampler = sdk_trace::AlwaysOffSamplerFactory::Create();
  auto sampler =
      sdk_trace::ParentBasedSamplerFactory::Create(std::move(fallback_sampler));

  // context
  return sdk_trace::TracerContextFactory::Create(std::move(processors),
                                                 resource, std::move(sampler));
}

auto create_ostream_context(opentelemetry::sdk::resource::Resource& resource,
                            std::ostream& file)
{
  namespace sdk_trace      = opentelemetry::sdk::trace;
  namespace exporter_trace = opentelemetry::exporter::trace;

  // ostream exporter
  auto exporter = exporter_trace::OStreamSpanExporterFactory::Create(file);
  
  // simple processor
  auto processor =
      sdk_trace::SimpleSpanProcessorFactory::Create(std::move(exporter));
  std::vector<std::unique_ptr<sdk_trace::SpanProcessor>> processors{};
  processors.push_back(std::move(processor));
  
  // always on sampler
  auto sampler = sdk_trace::AlwaysOnSamplerFactory::Create();
  
  // context
  return sdk_trace::TracerContextFactory::Create(std::move(processors),
                                                 resource, std::move(sampler));
}

} // namespace

TracerProvider::TracerProvider(std::string_view hostname,
                               std::string_view service_name,
                               std::string_view otlp_collector_endpoint)
{
  namespace sdk_resource = opentelemetry::sdk::resource;

  auto resource = [&service_name, &hostname]() {
    namespace semconv = opentelemetry::semconv;
    sdk_resource::ResourceAttributes attributes{
        {{semconv::service::kServiceName, std::string{service_name}},
         {semconv::host::kHostName, std::string{hostname}}}};
    return sdk_resource::Resource::Create(attributes);
  }();

  auto context = [&]() {
    std::string const prefix = "file://";
    if (otlp_collector_endpoint.starts_with(prefix)) {
      auto const file_path = otlp_collector_endpoint.substr(prefix.size());
      m_tracing_file.open(std::string{file_path});
      if (!m_tracing_file) {
        throw std::runtime_error(
            fmt::format("Failed to open tracing file '{}'", file_path));
      }
      return create_ostream_context(resource, m_tracing_file);
    }
    return create_otlp_context(resource, otlp_collector_endpoint);
  }();

  namespace sdk_trace = opentelemetry::sdk::trace;
  namespace trace     = opentelemetry::trace;
  m_provider = sdk_trace::TracerProviderFactory::Create(std::move(context));
  trace::Provider::SetTracerProvider(m_provider);

  namespace trace_prop   = opentelemetry::trace::propagation;
  namespace context_prop = opentelemetry::context::propagation;
  context_prop::GlobalTextMapPropagator::SetGlobalPropagator(
      std::shared_ptr<context_prop::TextMapPropagator>(
          new trace_prop::HttpTraceContext()));
}

TracerProvider::~TracerProvider()
{
  opentelemetry::trace::Provider::SetTracerProvider({});
}

opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer>
TracerProvider::get_tracer()
{
  auto provider = opentelemetry::trace::Provider::GetTracerProvider();
  return provider->GetTracer("storm-tracer");
}

} // namespace storm
