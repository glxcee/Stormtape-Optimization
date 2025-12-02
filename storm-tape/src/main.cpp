// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#include "app.hpp"
#include "configuration.hpp"
#include "database.hpp"
#include "database_soci.hpp"
#include "errors.hpp"
#include "local_storage.hpp"
#include "routes.hpp"
#include "tape_service.hpp"
#include "telemetry.hpp"
#include <boost/program_options.hpp>
#include <crow.h>
#include <fmt/core.h>
#include <soci/sqlite3/soci-sqlite3.h>
#include <filesystem>

namespace po = boost::program_options;
namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
  try {
    std::string config_file;
    po::options_description desc("Allowed options");

    // clang-format off
    desc.add_options()
    ("help,h", "produce help message")
    ("config,c",
     po::value<std::string>(&config_file)->default_value("storm-tape.conf"),
     "specify configuration file"
    );
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << "\n";
      return EXIT_SUCCESS;
    }

    auto const config = storm::load_configuration(fs::path{config_file});

    storm::CrowApp app;
    app.loglevel(crow::LogLevel{config.log_level});
    std::uint16_t concurrency = config.concurrency;
    soci::connection_pool db_pool{concurrency};
    for (std::size_t i{0}; i != concurrency; ++i) {
      db_pool.at(i).open(soci::sqlite3, "storm-tape.sqlite");
    }
    storm::SociDatabase db{db_pool};
    storm::LocalStorage storage{};
    storm::TapeService service{config, db, storage};
    storm::Telemetry telemetry{config};

    storm::create_routes(app, config, service);
    storm::create_internal_routes(app, config, service);

    // TODO add signals?
    app.port(config.port).concurrency(concurrency).run();

    for (std::size_t i{0}; i != concurrency; ++i) {
      db_pool.at(i).close();
    }

  } catch (std::exception const& e) {
    CROW_LOG_CRITICAL << fmt::format("Caught exception: {}", e.what());
    return EXIT_FAILURE;
  } catch (...) {
    CROW_LOG_CRITICAL << "Caught unknown exception";
    return EXIT_FAILURE;
  }
}

void boost::assertion_failed(char const* expr, char const* function,
                             char const* file, long line)
{
  std::cerr << "Failed assertion: '" << expr << "' in '" << function << "' ("
            << file << ':' << line << ")\n";
  std::abort();
}
