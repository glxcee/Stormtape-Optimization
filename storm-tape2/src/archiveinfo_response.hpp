// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#ifndef STORM_ARCHIVEINFO_RESPONSE_HPP
#define STORM_ARCHIVEINFO_RESPONSE_HPP

#include "types.hpp"
#include <boost/variant2.hpp>
#include <string>

namespace storm {

struct PathInfo
{
  LogicalPath path;
  boost::variant2::variant<Locality, std::string> info;
};

using PathInfos = std::vector<PathInfo>;

struct ArchiveInfoResponse
{
  PathInfos infos;
};

} // namespace storm

#endif
