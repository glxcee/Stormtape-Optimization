// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#ifndef STORM_CONFIGURATION_HPP
#define STORM_CONFIGURATION_HPP

#include "types.hpp"
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace storm {

struct StorageArea
{
  std::string name;
  PhysicalPath root;
  LogicalPaths access_points;
};

struct TelemetryConfiguration
{
  std::string service_name = "storm-tape";
  std::string tracing_endpoint;
};

using StorageAreas = std::vector<StorageArea>;
// log levels match Crow's
// 0 - Debug, 1 - Info, 2 - Warning, 3 - Error, 4 - Critical
using LogLevel = int;
struct Configuration
{
  std::string hostname = "localhost";
  std::uint16_t port   = 8080;
  StorageAreas storage_areas;
  LogLevel log_level                              = 1;
  bool mirror_mode                                = false;
  std::optional<TelemetryConfiguration> telemetry = std::nullopt;
  int concurrency                                 = 1;
};

Configuration load_configuration(std::istream& is);
Configuration load_configuration(fs::path const& p);

} // namespace storm

#endif
