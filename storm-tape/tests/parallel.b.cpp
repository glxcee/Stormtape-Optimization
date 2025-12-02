#include "configuration.hpp"
#include "errors.hpp"
#include "local_storage.hpp"
#include "tape_service_utils.hpp"
#include <boost/program_options.hpp>
#include <fmt/core.h>
#include <file.hpp>
#include <type_traits> // Necessario per std::is_void_v

namespace po = boost::program_options;
namespace fs = std::filesystem;

// FIX: Funzione benchmark generica che gestisce correttamente i ritorni void
template<typename Func>
auto benchmark(Func f)
{
  using Duration = std::chrono::duration<float, std::milli>;
  using ResultType = std::invoke_result_t<Func>;

  auto const t0 = std::chrono::steady_clock::now();

  if constexpr (std::is_void_v<ResultType>) {
      f(); // Esegue la funzione void
      auto const t1 = std::chrono::steady_clock::now();
      // Ritorna 0 come risultato finto per mantenere la struttura [res, time]
      return std::pair{0, std::chrono::duration_cast<Duration>(t1 - t0)};
  } else {
      auto result = f(); // Cattura il risultato
      auto const t1 = std::chrono::steady_clock::now();
      return std::pair{std::move(result), std::chrono::duration_cast<Duration>(t1 - t0)};
  }
}

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
    )
    ("parallel,p", "use parallel executor");
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << "\n";
      return EXIT_SUCCESS;
    }

    auto parallel = vm.count("parallel") > 0;

    auto const config = storm::load_configuration(fs::path{config_file});

    // Lettura dei percorsi (semplici stringhe/PhysicalPath)
    
    storm::PhysicalPaths paths;
    std::vector<storm::File> files;
    std::vector<storm::LogicalPath> logical_paths;
    for (std::string line; std::cin >> line;) {
      paths.push_back(line);

      logical_paths.push_back(line);

      storm::File f;
      f.physical_path = line;
      files.push_back(f);
    }
    std::cerr << "Read " << paths.size() << " files\n";

    storm::LocalStorage storage{};
    std::vector<std::pair<storm::PhysicalPath, storm::File::State>> files_to_update;

    parallel = false;
    std::cout << "Parallel mode: " << (parallel ? "enabled" : "disabled") << "\n";

    // --- BENCHMARK 1 ---
    auto [r1, t1] = benchmark([&] {
      // Usiamo paths_for_test1 con move
      return storm::extend_paths_with_localities(std::move(paths), storage,
                                                 parallel);
    });

    // --- BENCHMARK 2 ---
    auto [r2, t2] = benchmark([&] {
      // Passiamo il vettore di File corretto e le storage_areas
      storm::stage_path_resolver(files, config.storage_areas,
                                                 parallel);
    });

    auto [r3, t3] = benchmark([&] {
      // Passiamo il vettore di File corretto e le storage_areas
      storm::status_loop(files, storage, std::time(nullptr), files_to_update, parallel);
    });

    auto [r4, t4] = benchmark([&] {
      // Passiamo il vettore di File corretto e le storage_areas
      storm::archive_info_loop(logical_paths,config.storage_areas, storage, parallel);
    });

    std::cout << r1.size() << " files in " << t1.count() << "ms (extend_paths_with_localities)\n";
    std::cout << files.size() << " files in " << t2.count() << "ms (stage_path_resolver)\n";
    std::cout << files.size() << " files in " << t3.count() << "ms (status_loop)\n";
    std::cout << logical_paths.size() << " files in " << t4.count() << "ms (archive_info_loop)\n";

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