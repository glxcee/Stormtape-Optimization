// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#ifndef STORM_LOCALSTORAGE_HPP
#define STORM_LOCALSTORAGE_HPP

#include "storage.hpp"

namespace storm {

struct LocalStorage : Storage
{
  Result<bool> is_in_progress(PhysicalPath const& path) override;
  Result<FileSizeInfo> file_size_info(PhysicalPath const& path) override;
  Result<bool> is_on_tape(PhysicalPath const& path) override;
};

} // namespace storm

#endif
