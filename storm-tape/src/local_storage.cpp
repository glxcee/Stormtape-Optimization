// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#include "local_storage.hpp"
#include "extended_attributes.hpp"
#include "trace_span.hpp"
#include <sys/stat.h>

namespace storm {

Result<bool> LocalStorage::is_in_progress(PhysicalPath const& path)
{
  TRACE_FUNCTION();

  std::error_code ec;
  auto result = has_xattr(path, XAttrName{"user.TSMRecT"}, ec);
  if (ec == std::error_code{}) {
    return result;
  } else {
    return ec;
  }
}

Result<FileSizeInfo> LocalStorage::file_size_info(PhysicalPath const& path)
{
  TRACE_FUNCTION();

  struct stat sb = {};

  if (::stat(path.c_str(), &sb) == -1) {
    return std::make_error_code(std::errc{errno});
  }

  constexpr auto bytes_per_block{512L};
  return FileSizeInfo{static_cast<std::size_t>(sb.st_size),
                      sb.st_blocks * bytes_per_block < sb.st_size};
}

Result<bool> LocalStorage::is_on_tape(PhysicalPath const& path)
{
  TRACE_FUNCTION();

  std::error_code ec;
  auto result = has_xattr(path, XAttrName{"user.storm.migrated"}, ec);
  if (ec == std::error_code{}) {
    return result;
  } else {
    return ec;
  }
}

} // namespace storm
