// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#include "trace_span.hpp"

#include "http_text_map_carrier.hpp"
#include "io.hpp"
#include "telemetry_attributes.hpp"
#include "tracer_provider.hpp"

#include <opentelemetry/context/propagation/global_propagator.h>
#include <opentelemetry/context/runtime_context.h>
#include <opentelemetry/semconv/client_attributes.h>
#include <opentelemetry/semconv/incubating/http_attributes.h>
#include <opentelemetry/trace/span_startoptions.h>

namespace storm {

auto make_options(crow::request const& req)
{
  using namespace opentelemetry::context;
  using namespace opentelemetry::context::propagation;
  using namespace opentelemetry::trace;

  HttpTextMapCarrier<crow::ci_map> carrier{req.headers};
  auto prop            = GlobalTextMapPropagator::GetGlobalPropagator();
  auto current_context = RuntimeContext::GetCurrent();
  auto new_context     = prop->Extract(carrier, current_context);

  StartSpanOptions options;
  options.kind   = SpanKind::kServer;
  options.parent = GetSpan(new_context)->GetContext();
  return options;
}

TraceSpan::TraceSpan(std::string_view name)
{
  if (auto const tracer = TracerProvider::get_tracer()) {
    m_span  = tracer->StartSpan(name);
    m_scope = std::make_unique<opentelemetry::trace::Scope>(
        tracer->WithActiveSpan(m_span));
  }
}

TraceSpan::TraceSpan(std::string_view name, crow::request const& req)
{
  if (auto const tracer = TracerProvider::get_tracer()) {
    m_span  = tracer->StartSpan(name, to_semconv(req), make_options(req));
    m_scope = std::make_unique<opentelemetry::trace::Scope>(
        tracer->WithActiveSpan(m_span));
  }
}

TraceSpan::TraceSpan(std::string_view name, crow::request const& req,
                     std::string_view operation)
{
  if (auto const tracer = TracerProvider::get_tracer()) {
    m_span =
        tracer->StartSpan(name, to_semconv(req, operation), make_options(req));
    m_scope = std::make_unique<opentelemetry::trace::Scope>(
        tracer->WithActiveSpan(m_span));
  }
}

TraceSpan::~TraceSpan()
{
  try {
    if (m_span) {
      m_span->End();
    }
  } catch (...) {
  }
}

} // namespace storm
