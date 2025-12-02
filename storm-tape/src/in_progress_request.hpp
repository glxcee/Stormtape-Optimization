// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#ifndef STORM_IN_PROGRESS_REQUEST_HPP
#define STORM_IN_PROGRESS_REQUEST_HPP

#include <cstddef>

namespace storm {

struct InProgressRequest
{
  inline static constexpr struct Tag {} tag{};
  std::size_t n_files{1'000};
  int precise{0};
};

} // namespace storm
#endif
