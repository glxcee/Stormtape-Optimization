// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#include "configuration.hpp"
#include "extended_attributes.hpp"
#include "uuid_generator.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <boost/url.hpp>
#include <crow/logging.h>
#include <fmt/std.h>
#include <yaml-cpp/yaml.h>

#include <unistd.h>
#include <algorithm>
#include <array>
#include <climits>
#include <fstream>
#include <iostream>
#include <numeric>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <system_error>

namespace algo = boost::algorithm;

namespace storm {

void set_hostname(Configuration& config)
{
  std::array<char, HOST_NAME_MAX> buffer{};
  auto err      = ::gethostname(buffer.data(), buffer.size());
  buffer.back() = '\0';
  std::string tmp{buffer.data(), std::strlen(buffer.data())};
  if (err != 0) {
    auto errc = std::make_error_code(std::errc(errno));
    if (errc == std::errc::filename_too_long) {
      CROW_LOG_WARNING << fmt::format("hostname has been truncated to '{}'",
                                      tmp);
    } else {
      throw std::system_error(errc, "set_hostname");
    }
  }
  config.hostname = std::move(tmp);
}

static std::string load_storage_area_name(YAML::Node const& name)
{
  if (!name.IsDefined()) {
    throw std::runtime_error{"there is a storage area with no name"};
  }

  if (name.IsNull()) {
    throw std::runtime_error{"there is a storage area with an empty name"};
  }

  auto result = name.as<std::string>();

  if (result.empty()) {
    throw std::runtime_error{
        "there is a storage area with an empty string name"};
  }

  static std::regex const re{"^[a-zA-Z][a-zA-Z0-9-_.]*$"};

  if (std::regex_match(result, re)) {
    return result;
  } else {
    throw std::runtime_error{
        fmt::format("invalid storage area name '{}'", result)};
  }
}

static bool storm_has_all_permissions(fs::path const& path)
{
  UuidGenerator uuid_gen;
  auto tmp = path / uuid_gen();
  std::shared_ptr<void> guard{nullptr, [&](auto) {
                                std::error_code ec;
                                fs::remove(tmp, ec);
                              }};
  {
    // can create a file
    std::ofstream f(tmp);
    if (!f) {
      return false;
    }
  }
  {
    // can read a file
    std::ifstream f(tmp);
    if (!f) {
      return false;
    }
  }

  {
    XAttrName const tsm_rect{"user.TSMRecT"};
    std::error_code ec;

    // can create an xattr
    create_xattr(tmp, tsm_rect, ec);
    if (ec != std::error_code{}) {
      return false;
    }

    // can check an xattr
    has_xattr(tmp, tsm_rect, ec);
    if (ec != std::error_code{}) {
      return false;
    }
  }

  return true;
}

static PhysicalPath load_storage_area_root(YAML::Node const& root,
                                           std::string_view sa_name)
{
  if (!root.IsDefined()) {
    throw std::runtime_error{
        fmt::format("storage area '{}' has no root", sa_name)};
  }

  if (root.IsNull()) {
    throw std::runtime_error{
        fmt::format("storage area '{}' has an empty root", sa_name)};
  }

  fs::path root_path{root.as<std::string>("")};

  return PhysicalPath{root_path.lexically_normal()};
}

static LogicalPaths load_storage_area_access_points(YAML::Node const& node,
                                                    std::string_view sa_name)
{
  if (!node.IsDefined()) {
    throw std::runtime_error{
        fmt::format("storage area '{}' has no access-point", sa_name)};
  }

  if (node.IsNull()) {
    throw std::runtime_error{
        fmt::format("storage area '{}' has an empty access-point", sa_name)};
  }

  if (LogicalPath access_point{node.as<std::string>("")};
      !access_point.empty()) {
    return {LogicalPath{access_point.lexically_normal()}};
  }

  using Strings = std::vector<std::string>;
  auto strings  = node.as<Strings>(Strings{});
  LogicalPaths paths;
  paths.reserve(strings.size());
  std::transform(strings.begin(), strings.end(), std::back_inserter(paths),
                 [](auto& s) { return LogicalPath{std::move(s)}; });
  return paths;
}

static StorageArea load_storage_area(YAML::Node const& sa)
{
  std::string name = load_storage_area_name(sa["name"]);
  PhysicalPath root{load_storage_area_root(sa["root"], name)};
  LogicalPaths ap{load_storage_area_access_points(sa["access-point"], name)};
  return {name, root, ap};
}

static void check_root(StorageArea const& sa, bool mirror_mode)
{
  auto const& [name, root, _] = sa;

  if (!root.is_absolute()) {
    throw std::runtime_error{fmt::format(
        "root '{}' of storage area '{}' is not an absolute path", root, name)};
  }

  std::error_code ec;
  auto const stat = fs::status(root, ec);

  if (!fs::exists(stat)) {
    throw std::runtime_error{fmt::format(
        "root '{}' of storage area '{}' does not exist", root, name)};
  }

  if (ec != std::error_code{}) {
    throw std::runtime_error{fmt::format(
        "root '{}' of storage area '{}' is not available", root, name)};
  }
  if (!fs::is_directory(stat)) {
    throw std::runtime_error{fmt::format(
        "root '{}' of storage area '{}' is not a directory", root, name)};
  }

  if (!mirror_mode && !storm_has_all_permissions(root)) {
    throw std::runtime_error{fmt::format(
        "root '{}' of storage area '{}' has invalid permissions", root, name)};
  }
}

static StorageAreas load_storage_areas(YAML::Node const& sas)
{
  StorageAreas result;

  for (auto& sa : sas) {
    result.emplace_back(load_storage_area(sa));
  }

  if (result.empty()) {
    throw std::runtime_error{
        "configuration error - empty 'storage-areas' entry"};
  }

  // keep storage areas sorted by name
  std::sort(result.begin(),
            result.end(), //
            [](StorageArea const& l, StorageArea const& r) {
              return algo::ilexicographical_compare(l.name, r.name);
            });

  // copy all the access points in another vector, together with a pointer to
  // the corresponding storage area
  auto const access_points = [&] {
    using Aps = std::vector<std::pair<LogicalPath, StorageArea const*>>;
    auto aps  = std::transform_reduce(
        result.begin(), result.end(), Aps{},
        [](Aps r1, Aps r2) {
          r1.reserve(r1.size() + r2.size());
          std::move(r2.begin(), r2.end(), std::back_inserter(r1));
          return r1;
        },
        [](auto& sa) {
          Aps partial;
          auto const& sa_aps = sa.access_points;
          partial.reserve(sa_aps.size());
          std::transform(sa_aps.begin(), sa_aps.end(),
                          std::back_inserter(partial),
                          [&](auto& ap) { return std::pair{ap, &sa}; });
          return partial;
        });
    std::sort(aps.begin(), aps.end(),
              [](auto const& a, auto const& b) { return a.first < b.first; });
    return aps;
  }();

  {
    // storage areas cannot have the same name
    auto it =
        std::adjacent_find(result.begin(),
                           result.end(), //
                           [](StorageArea const& l, StorageArea const& r) {
                             return algo::iequals(l.name, r.name);
                           });

    if (it != result.end()) {
      throw std::runtime_error{
          fmt::format("two storage areas have the same name '{}'", it->name)};
    }
  }

  {
    // two storage areas cannot have an access point in common
    auto it = std::adjacent_find(
        access_points.begin(),
        access_points.end(), //
        [](auto const& l, auto const& r) { return l.first == r.first; });

    if (it != access_points.end()) {
      throw std::runtime_error{fmt::format(
          "storage areas '{}' and '{}' have the access point '{}' in common",
          it->second->name, std::next(it)->second->name, it->first)};
    }
  }

  {
    // all access points must be absolute
    auto it = std::find_if(access_points.begin(), access_points.end(),
                           [](auto const& e) { return e.first.is_relative(); });

    if (it != access_points.end()) {
      throw std::runtime_error{fmt::format(
          "access point '{}' of storage area '{}' is not an absolute path",
          it->first, it->second->name)};
    }
  }

  return result;
}

static std::optional<std::uint16_t> load_port(YAML::Node const& node)
{
  if (!node.IsDefined()) {
    return {};
  }

  if (node.IsNull()) {
    throw std::runtime_error{fmt::format("port is null")};
  }

  int port;
  if (boost::conversion::try_lexical_convert(node, port)) {
    if (port > 0 && port < 65536) {
      return static_cast<std::uint16_t>(port);
    }
  }
  throw std::runtime_error{"invalid 'port' entry in configuration"};
}

static std::optional<LogLevel> load_log_level(YAML::Node const& node)
{
  if (!node.IsDefined()) {
    return {};
  }

  if (node.IsNull()) {
    throw std::runtime_error{fmt::format("log-level is null")};
  }

  int log_level;
  if (boost::conversion::try_lexical_convert(node, log_level)) {
    if (log_level >= 0 && log_level <= 4) {
      return log_level;
    }
  }
  throw std::runtime_error{"invalid 'log-level' entry in configuration"};
}

static std::optional<LogLevel> load_concurrency(YAML::Node const& node)
{
  if (!node.IsDefined()) {
    return {};
  }

  if (node.IsNull()) {
    throw std::runtime_error{fmt::format("concurrency is null")};
  }

  int concurrency;
  if (boost::conversion::try_lexical_convert(node, concurrency)) {
    if (concurrency > 0) {
      return concurrency;
    }
  }
  throw std::runtime_error{"invalid 'log-level' entry in configuration"};
}

static std::optional<bool> load_mirror_mode(YAML::Node const& node)
{
  if (!node.IsDefined()) {
    return {};
  }

  if (node.IsNull()) {
    throw std::runtime_error{fmt::format("mirror-mode is null")};
  }

  bool value;
  if (YAML::convert<bool>::decode(node, value)) {
    return value;
  }
  throw std::runtime_error{"invalid 'mirror-mode' entry in configuration"};
}

static std::optional<std::string> load_service_name(YAML::Node const& node)
{
  const auto service_name_key = "service-name";
  const auto& value           = node[service_name_key];
  if (!value.IsDefined()) {
    return std::nullopt;
  }

  if (value.IsNull()) {
    throw std::runtime_error{fmt::format("'{}' is null", service_name_key)};
  }

  auto const service_name = value.as<std::string>("");
  if (service_name.empty()) {
    throw std::runtime_error{
        fmt::format("'{}' is not a valid string", service_name_key)};
  }
  return service_name;
}

static std::optional<std::string> load_tracing_endpoint(YAML::Node const& node)
{
  const auto tracing_key = "tracing-endpoint";
  const auto& value      = node[tracing_key];
  if (!value.IsDefined()) {
    return std::nullopt;
  }

  if (value.IsNull()) {
    throw std::runtime_error{fmt::format("'{}' is null", tracing_key)};
  }

  const auto endpoint = value.as<std::string>();
  const auto result   = boost::urls::parse_uri(endpoint);
  if (result.has_error()) {
    throw std::runtime_error{
        fmt::format("'{}' is not a valid uri", tracing_key)};
  }

  auto const url_view = result.value();
  auto const scheme   = url_view.scheme();

  if (scheme != "http" && scheme != "https" && scheme != "file") {
    throw std::runtime_error{
        fmt::format("'{}' uri scheme '{}' is not valid", tracing_key, scheme)};
  }

  if ((scheme == "http" || scheme == "https") && //
      url_view.host_name().empty() &&            //
      url_view.host_address().empty()) {
    throw std::runtime_error{
        fmt::format("'{}' has no hostname or address", tracing_key)};
  }

  return endpoint;
}

static std::optional<TelemetryConfiguration>
load_telemetry(YAML::Node const& node)
{
  if (!node.IsDefined() || node.IsNull()) {
    return std::nullopt;
  }

  TelemetryConfiguration config;

  const auto& maybe_service_name = load_service_name(node);
  if (maybe_service_name.has_value()) {
    config.service_name = std::move(maybe_service_name.value());
  }

  const auto& maybe_tracing_endpoint = load_tracing_endpoint(node);
  if (maybe_tracing_endpoint.has_value()) {
    config.tracing_endpoint = std::move(maybe_tracing_endpoint.value());
  }

  return config;
}

static Configuration load(YAML::Node const& node)
{
  const auto sas_key = "storage-areas";
  const auto& sas    = node[sas_key];

  if (!sas) {
    throw std::runtime_error{
        fmt::format("no '{}' entry in configuration", sas_key)};
  }

  Configuration config;
  set_hostname(config);
  config.storage_areas = load_storage_areas(sas);

  auto const port_key   = "port";
  auto const& port_s    = node[port_key];
  auto const maybe_port = load_port(port_s);
  if (maybe_port.has_value()) {
    config.port = *maybe_port;
  }

  auto const log_level_key   = "log-level";
  auto const& log_level_s    = node[log_level_key];
  auto const maybe_log_level = load_log_level(log_level_s);
  if (maybe_log_level.has_value()) {
    config.log_level = *maybe_log_level;
  }

  {
    auto const key    = "mirror-mode";
    auto const& value = node[key];
    auto const maybe  = load_mirror_mode(value);
    if (maybe.has_value()) {
      config.mirror_mode = *maybe;
    }
  }

  {
    auto const key    = "telemetry";
    auto const& value = node[key];
    config.telemetry  = load_telemetry(value);
  }

  auto const concurrency_key   = "concurrency";
  auto const& concurrency_s    = node[concurrency_key];
  auto const maybe_concurrency = load_concurrency(concurrency_s);
  if (maybe_concurrency.has_value()) {
    config.concurrency = *maybe_concurrency;
  }

  return config;
}

auto check_sa_roots(StorageAreas const& sas, bool mirror_mode)
{
  std::for_each(sas.begin(), sas.end(),
                [=](auto& sa) { check_root(sa, mirror_mode); });
}

Configuration load_configuration(std::istream& is)
{
  YAML::Node const node = YAML::Load(is);
  auto config           = load(node);
  check_sa_roots(config.storage_areas, config.mirror_mode);
  return config;
}

Configuration load_configuration(fs::path const& p)
{
  std::ifstream is(p);
  if (!is) {
    throw std::runtime_error{
        fmt::format("cannot open configuration file '{}'", p)};
  } else {
    return load_configuration(is);
  }
}

} // namespace storm
