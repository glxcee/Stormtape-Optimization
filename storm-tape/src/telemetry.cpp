// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#include "telemetry.hpp"
#include "configuration.hpp"

namespace storm {

Telemetry::Telemetry(Configuration const& config)
{
  if (config.telemetry) {
    auto const& tel_config = config.telemetry.value();
    if (!tel_config.tracing_endpoint.empty()) {
      m_tracer_provider.emplace(config.hostname, tel_config.service_name,
                                tel_config.tracing_endpoint);
    }
  }
}

} // namespace storm
