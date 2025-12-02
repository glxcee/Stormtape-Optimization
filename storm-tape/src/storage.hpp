// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#ifndef STORM_STORAGE_HPP
#define STORM_STORAGE_HPP

#include "types.hpp"

namespace storm {

struct Storage
{
  virtual ~Storage()                                            = default;
  virtual Result<bool> is_in_progress(PhysicalPath const& path) = 0;
  virtual Result<FileSizeInfo> file_size_info(PhysicalPath const& path) = 0;
  virtual Result<bool> is_on_tape(PhysicalPath const& path)             = 0;
};

} // namespace storm

#endif
