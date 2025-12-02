// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#ifndef STORM_TAKEOVER_REQUEST_HPP
#define STORM_TAKEOVER_REQUEST_HPP

#include <cstddef>

namespace storm {

struct TakeOverRequest
{
  inline static constexpr struct Tag {} tag{};
  static constexpr std::size_t min_n_files{1};
  static constexpr std::size_t max_n_files{1'000'000};
  std::size_t n_files;
};

} // namespace storm
#endif
