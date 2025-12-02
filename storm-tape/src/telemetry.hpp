// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#ifndef STORM_TELEMETRY_HPP
#define STORM_TELEMETRY_HPP

#include "tracer_provider.hpp"

#include <optional>

namespace storm {

class Configuration;
class Telemetry
{
  std::optional<TracerProvider> m_tracer_provider = std::nullopt;

 public:
  explicit Telemetry(Configuration const& config);
  ~Telemetry()                           = default;
  Telemetry(const Telemetry&)            = delete;
  Telemetry(Telemetry&&)                 = delete;
  Telemetry& operator=(const Telemetry&) = delete;
  Telemetry& operator=(Telemetry&&)      = delete;
};

} // namespace storm

#endif
