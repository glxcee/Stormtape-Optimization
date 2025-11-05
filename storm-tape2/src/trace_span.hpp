// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#ifndef STORM_TRACE_SPAN_HPP
#define STORM_TRACE_SPAN_HPP

#include <crow/http_request.h>
#include <opentelemetry/trace/scope.h>
#include <opentelemetry/trace/span.h>
#include <memory>
#include <string_view>

#define COMBINE_HELPER(X, Y) X##Y
#define COMBINE(X, Y)        COMBINE_HELPER(X, Y)
#define TRACE_SCOPE(name)    storm::TraceSpan COMBINE(span, __LINE__)(name);
#define TRACE_FUNCTION()     TRACE_SCOPE(__PRETTY_FUNCTION__)

namespace storm {

class TraceSpan
{
  std::shared_ptr<opentelemetry::trace::Span> m_span;
  std::unique_ptr<opentelemetry::trace::Scope> m_scope;

 public:
  explicit TraceSpan(std::string_view name);
  TraceSpan(std::string_view name, crow::request const& req);
  TraceSpan(std::string_view name, crow::request const& req,
       std::string_view operation);
  ~TraceSpan();
};

} // namespace storm

#endif
