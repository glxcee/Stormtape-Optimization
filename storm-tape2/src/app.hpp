// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#ifndef STORM_APP_HPP
#define STORM_APP_HPP

#include "access_logger.hpp"
#include <crow.h>

namespace storm {
using CrowApp = crow::App<AccessLogger>;
} // namespace storm

#endif
