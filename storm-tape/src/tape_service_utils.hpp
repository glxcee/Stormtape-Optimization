#ifndef STORM_TAPE_SERVICE_UTILS_HPP
#define STORM_TAPE_SERVICE_UTILS_HPP

#include "extended_attributes.hpp"
#include "extended_file_status.hpp"
#include "storage.hpp"
#include "trace_span.hpp"
#include "types.hpp"
#include "storage_area_resolver.hpp" 
#include <crow/logging.h>
#include <fmt/std.h>
#include <execution>
#include <span>
#include <filesystem>
#include <ctime> 
#include <vector>
#include "file.hpp"
#include "archiveinfo_response.hpp"

namespace storm {

namespace fs = std::filesystem;

using PathLocality = std::pair<PhysicalPath, Locality>;

inline bool recall_in_progress(PhysicalPath const& path)
{
  XAttrName const tsm_rect{"user.TSMRecT"};
  std::error_code ec;
  auto const in_progress = has_xattr(path, tsm_rect, ec);
  return ec == std::error_code{} && in_progress;
}

inline bool override_locality(Locality& locality, PhysicalPath const& path)
{
  if (locality == Locality::lost) {
    CROW_LOG_ERROR << fmt::format(
        "The file {} appears lost, check stubbification and presence of "
        "user.storm.migrated xattr",
        path);
    // do not scare the client
    locality = Locality::unavailable;
    return true;
  }
  return false;
}

inline auto extend_paths_with_localities(PhysicalPaths&& paths,
                                         Storage& storage,
                                         bool parallel = false)
{
  TRACE_FUNCTION();
  std::vector<PathLocality> path_localities;

  if (parallel) {
    path_localities.resize(paths.size());

    std::transform(std::execution::par, paths.begin(), paths.end(),
                   path_localities.begin(), [&](PhysicalPath& path) {
                     auto const locality =
                         ExtendedFileStatus{storage, path}.locality();
                     return PathLocality{std::move(path), locality};
                   });
  } else {
    path_localities.reserve(paths.size());

    for (auto&& path : paths) {
      auto const locality = ExtendedFileStatus{storage, path}.locality();
      path_localities.emplace_back(std::move(path), locality);
    }
  }
  return path_localities;
}

inline auto stage_path_resolver(auto &files, const auto &storage_areas, bool parallel = false) {
  if(parallel) {
    std::for_each(std::execution::par, files.begin(), files.end(), 
      [&storage_areas](auto &file) {
        StorageAreaResolver resolver{storage_areas};
        file.physical_path = resolver(file.logical_path);
        std::error_code ec;
        auto status = fs::status(file.physical_path, ec);
        if (ec || !fs::is_regular_file(status)) {
          //using FileType = std::remove_reference_t<decltype(file)>;
          file.state       = File::State::failed;
          file.started_at  = std::time(nullptr);
          file.finished_at = file.started_at;
        }
      }
    );
  }
  else {
    StorageAreaResolver resolve{storage_areas};
    for (auto& file : files) {
      file.physical_path = resolve(file.logical_path);
      std::error_code ec;
      auto status = fs::status(file.physical_path, ec);
      if (ec || !fs::is_regular_file(status)) {
        //using FileType = std::remove_reference_t<decltype(file)>;
        file.state       = File::State::failed;
        file.started_at  = std::time(nullptr);
        file.finished_at = file.started_at;
      }
    }
  }
}


inline auto status_loop(auto &files, auto &storage, auto now, auto &files_to_update, bool parallel = false) {

  if (parallel) {
    
    std::for_each(std::execution::par, files.begin(), files.end(),
      [&storage, &now, &files_to_update](auto &file) {
        ExtendedFileStatus file_status{storage, file.physical_path};

        switch (file.state) {
        
        case File::State::started: {
          std::cout<<"STARTED \n";
          if (file_status.is_in_progress()) {
            break;
          }
          file.state       = file_status.is_stub() ? File::State::failed
                                                   : File::State::completed;
          file.finished_at = now;
          files_to_update.emplace_back(file.physical_path, file.state);
          break;
        }

        case File::State::completed:
        case File::State::cancelled:
        case File::State::failed:
          // do nothing
          break;

        case File::State::submitted: {
          std::cout<<"SUBMITTED \n";
          if (file_status && file_status.is_in_progress()) {
            file.state      = File::State::started;
            file.started_at = now;
            files_to_update.emplace_back(file.physical_path,
                                         File::State::started);
          } else if (file_status && !file_status.is_stub()) {
            file.state       = File::State::completed;
            file.started_at  = now;
            file.finished_at = now;
            files_to_update.emplace_back(file.physical_path, file.state);
          } else if (!file_status) {
            file.state       = File::State::failed;
            file.started_at  = now;
            file.finished_at = now;
            files_to_update.emplace_back(file.physical_path, file.state);
          }
          break;
        }
        }
      });

  } else {
    for (auto& file : files) {
      ExtendedFileStatus file_status{storage, file.physical_path};

      switch (file.state) {
        
      case File::State::started: {
        std::cout<<"STARTED \n";
        if (file_status.is_in_progress()) {
          break;
        }
        file.state       = file_status.is_stub() ? File::State::failed
                                                 : File::State::completed;
        file.finished_at = now;
        files_to_update.emplace_back(file.physical_path, file.state);
        break;
      }

      case File::State::completed:
      case File::State::cancelled:
      case File::State::failed:
        // do nothing
        break;

      case File::State::submitted: {
        std::cout<<"SUBMITTED \n";
        if (file_status && file_status.is_in_progress()) {
          file.state      = File::State::started;
          file.started_at = now;
          files_to_update.emplace_back(file.physical_path,
                                       File::State::started);
        } else if (file_status && !file_status.is_stub()) {
          file.state       = File::State::completed;
          file.started_at  = now;
          file.finished_at = now;
          files_to_update.emplace_back(file.physical_path, file.state);
        } else if (!file_status) {
          file.state       = File::State::failed;
          file.started_at  = now;
          file.finished_at = now;
          files_to_update.emplace_back(file.physical_path, file.state);
        }
        break;
      }
      }
    }
  }
  return files_to_update;
}

inline auto archive_info_loop(auto &paths, const auto &storage_areas, auto &storage, bool parallel = false) {
  PathInfos infos;
  if(parallel) {
    
    // 1. Pre-allocazione del vettore di output per l'accesso concorrente
    infos.resize(paths.size());

    // 2. Trasformazione parallela
    std::transform(std::execution::par, paths.begin(), paths.end(), infos.begin(),
        [&](auto& logical_path) {
          using namespace std::string_literals;
          
          // Resolver locale al thread
          StorageAreaResolver resolve{storage_areas};

          auto const physical_path = resolve(logical_path);
          std::error_code ec;
          auto status = fs::status(physical_path, ec);

          if (!fs::exists(status)) {
            // std::move su logical_path (input) va bene perché è una trasformazione 1:1
            return PathInfo{std::move(logical_path), "No such file or directory"s};
          }
          if (ec != std::error_code{}) {
            return PathInfo{std::move(logical_path), Locality::unavailable};
          }
          if (fs::is_directory(status)) {
            return PathInfo{std::move(logical_path), "Is a directory"s};
          }
          if (!fs::is_regular_file(status)) {
            return PathInfo{std::move(logical_path), "Not a regular file"s};
          }
          
          auto locality = ExtendedFileStatus{storage, physical_path}.locality();
          override_locality(locality, physical_path);
          
          return PathInfo{std::move(logical_path), locality};
        }
    );

  } else {
    
    infos.reserve(paths.size());

    StorageAreaResolver resolve{storage_areas};

    std::transform( //
        paths.begin(), paths.end(), std::back_inserter(infos),
        [&](LogicalPath& logical_path) {
          using namespace std::string_literals;

          auto const physical_path = resolve(logical_path);
          std::error_code ec;
          auto status = fs::status(physical_path, ec);

          // if the file doesn't exist, fs::status sets ec, so check first for
          // existence
          if (!fs::exists(status)) {
            return PathInfo{std::move(logical_path),
                            "No such file or directory"s};
          }
          if (ec != std::error_code{}) {
            return PathInfo{std::move(logical_path), Locality::unavailable};
          }
          if (fs::is_directory(status)) {
            return PathInfo{std::move(logical_path), "Is a directory"s};
          }
          if (!fs::is_regular_file(status)) {
            return PathInfo{std::move(logical_path), "Not a regular file"s};
          }
          auto locality = ExtendedFileStatus{storage, physical_path}.locality();
          override_locality(locality, physical_path);
          return PathInfo{std::move(logical_path), locality};
        });
      
      

  }
  return ArchiveInfoResponse{infos};
}


inline auto select_only_on_tape(
    std::span<PathLocality> path_locs) //-V813 span is passed by value
{
  TRACE_FUNCTION();
  auto const it = std::partition(path_locs.begin(), path_locs.end(),
                                 [&](auto const& path_loc) {
                                   return path_loc.second == Locality::tape
                                       // let's try also apparently-lost files
                                       || path_loc.second == Locality::lost;
                                 });
  return std::tuple{std::span{path_locs.begin(), it},
                    std::span{it, path_locs.end()}};
}

inline auto select_in_progress(std::span<PathLocality> path_locs)
{
  TRACE_FUNCTION();
  auto const it = std::partition(
      path_locs.begin(), path_locs.end(),
      [](auto const& path_loc) { return recall_in_progress(path_loc.first); });
  return std::tuple{std::span{path_locs.begin(), it},
                    std::span{it, path_locs.end()}};
}

inline auto select_on_disk(
    std::span<PathLocality> path_locs) //-V813 span is passed by value
{
  TRACE_FUNCTION();
  auto const it = std::partition(
      path_locs.begin(), path_locs.end(), [](auto const& path_loc) {
        return path_loc.second == Locality::disk
            || path_loc.second == Locality::disk_and_tape;
      });
  return std::tuple{std::span{path_locs.begin(), it},
                    std::span{it, path_locs.end()}};
}

} // namespace storm

#endif