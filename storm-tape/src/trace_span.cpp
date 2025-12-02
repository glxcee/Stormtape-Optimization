// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#include "trace_span.hpp"

#include "http_text_map_carrier.hpp"
#include "io.hpp"
#include "telemetry_attributes.hpp"
#include "tracer_provider.hpp"

#include <boost/assert.hpp>
#include <opentelemetry/context/propagation/global_propagator.h>
#include <opentelemetry/context/runtime_context.h>
#include <opentelemetry/semconv/client_attributes.h>
#include <opentelemetry/semconv/incubating/http_attributes.h>
#include <opentelemetry/trace/span_startoptions.h>

namespace storm {

namespace {

auto make_options(crow::request const& req)
{
  namespace propagation = opentelemetry::context::propagation;
  HttpTextMapCarrier<crow::ci_map> carrier{req.headers};
  auto prop = propagation::GlobalTextMapPropagator::GetGlobalPropagator();
  auto current_context = opentelemetry::context::RuntimeContext::GetCurrent();
  auto new_context     = prop->Extract(carrier, current_context);

  opentelemetry::trace::StartSpanOptions options;
  options.kind   = opentelemetry::trace::SpanKind::kServer;
  options.parent = opentelemetry::trace::GetSpan(new_context)->GetContext();
  return options;
}
} // namespace

TraceSpan::TraceSpan(std::string_view name)
{
  auto const tracer = TracerProvider::get_tracer();
  BOOST_ASSERT(tracer);
  m_span = tracer->StartSpan({name.data(), name.size()});
  BOOST_ASSERT(m_span);
  m_scope = std::make_unique<opentelemetry::trace::Scope>(
      tracer->WithActiveSpan(m_span));
}

TraceSpan::TraceSpan(std::string_view name, crow::request const& req)
{
  auto const tracer = TracerProvider::get_tracer();
  BOOST_ASSERT(tracer);
  m_span = tracer->StartSpan({name.data(), name.size()}, to_semconv(req),
                             make_options(req));
  BOOST_ASSERT(m_span);
  m_scope = std::make_unique<opentelemetry::trace::Scope>(
      tracer->WithActiveSpan(m_span));
}

TraceSpan::TraceSpan(std::string_view name, crow::request const& req,
                     std::string_view operation)
{
  auto const tracer = TracerProvider::get_tracer();
  BOOST_ASSERT(tracer);
  m_span = tracer->StartSpan({name.data(), name.size()}, to_semconv(req),
                             make_options(req));
  BOOST_ASSERT(m_span);
  m_span->SetAttribute(
      OtelAttribute::operation_name,
      opentelemetry::nostd::string_view{operation.data(), operation.size()});
  m_scope = std::make_unique<opentelemetry::trace::Scope>(
      tracer->WithActiveSpan(m_span));
}

TraceSpan::~TraceSpan()
{
  try {
    m_span->End();
  } catch (...) {
  }
}

void TraceSpan::set_batch_size(std::size_t size)
{
  m_span->SetAttribute(OtelAttribute::batch_size, size);
}

} // namespace storm
