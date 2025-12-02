// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#ifndef STORM_TAPE_API_HPP
#define STORM_TAPE_API_HPP

#include "app.hpp"
#include <crow.h>

namespace storm {
class Configuration;
class Database;
class TapeService;

void create_routes(CrowApp& app, storm::Configuration const& config,
                   storm::TapeService& service);
void create_internal_routes(CrowApp& app,
                            storm::Configuration const& config,
                            storm::TapeService& service);
} // namespace storm

#endif
