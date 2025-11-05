// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: Apache-2.0

#ifndef STORM_EXTENDED_FILE_STATUS_HPP
#define STORM_EXTENDED_FILE_STATUS_HPP

#include "storage.hpp"
#include "types.hpp"

#include <cstddef>
#include <optional>
#include <system_error>

namespace storm {

class ExtendedFileStatus
{
  Storage& m_storage;
  PhysicalPath const& m_path;
  std::error_code m_ec{};
  template<typename T>
  using Cached = std::optional<T>;
  Cached<bool> m_in_progress{std::nullopt};
  Cached<FileSizeInfo> m_file_size_info{std::nullopt};
  Cached<bool> m_on_tape{std::nullopt};

 public:
  ExtendedFileStatus(Storage& storage, PhysicalPath const& path)
      : m_storage(storage)
      , m_path(path)
  {}
  auto error() const noexcept
  {
    return m_ec;
  }
  explicit operator bool() const noexcept
  {
    return m_ec == std::error_code{};
  }
  bool is_in_progress();
  bool is_stub();
  bool is_on_tape();
  std::size_t file_size();
  Locality locality();
};

} // namespace storm

#endif // STORM_EXTENDED_FILE_STATUS_HPP
