// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#ifndef STORM_TYPES_HPP
#define STORM_TYPES_HPP

#include <boost/system.hpp>
#include <filesystem>
#include <string>
#include <type_traits>
#include <vector>

namespace storm {

namespace fs = std::filesystem;

inline fs::path const& format_as(fs::path const& p)
{
  return p;
}

struct LogicalPath : fs::path
{
  using fs::path::path;
};

struct PhysicalPath : fs::path
{
  using fs::path::path;
};

using LogicalPaths  = std::vector<LogicalPath>;
using PhysicalPaths = std::vector<PhysicalPath>;
using TimePoint     = long long int;
using StageId       = std::string;

enum class Locality : unsigned char
{
  unavailable, // this is zero to optimize the initialization of Localities
  disk,
  tape,
  disk_and_tape,
  lost,
  none
};

std::string to_string(Locality locality);

template<class Enum>
constexpr std::underlying_type_t<Enum> to_underlying(Enum e) noexcept
{
  return static_cast<std::underlying_type_t<Enum>>(e);
}

template<typename T>
using Result = boost::system::result<T>;

struct FileSizeInfo
{
  std::size_t size{0};
  bool is_stub{false};
};

} // namespace storm

#endif
