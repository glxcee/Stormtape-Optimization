// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#ifndef STORM_CANCEL_RESPONSE_HPP
#define STORM_CANCEL_RESPONSE_HPP

#include "types.hpp"

namespace storm {

struct CancelResponse
{
  StageId id{};
  LogicalPaths invalid{};

  explicit CancelResponse(StageId i, LogicalPaths v = {})
      : id(std::move(i))
      , invalid(std::move(v))
  {}
};

} // namespace storm

#endif
