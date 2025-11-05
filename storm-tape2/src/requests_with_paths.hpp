// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#ifndef STORM_REQUEST_WITH_PATHS_HPP
#define STORM_REQUEST_WITH_PATHS_HPP

#include "types.hpp"

namespace storm {

struct RequestWithPaths
{
  struct Tag {};
  static constexpr Tag tag{};

  LogicalPaths paths;
};

struct CancelRequest : RequestWithPaths
{
};

struct ReleaseRequest : RequestWithPaths
{
};

struct ArchiveInfoRequest : RequestWithPaths
{
};

} // namespace storm

#endif
