# SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
#
# SPDX-License-Identifier: EUPL-1.2

cmake_minimum_required(VERSION 3.16)

if (NOT DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "You must set VCPKG_ROOT env variable or pass CMAKE_TOOLCHAIN_FILE")
endif()

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

file(STRINGS version.txt storm_tape_version LIMIT_COUNT 1)
string(STRIP "${storm_tape_version}" storm_tape_version)
message("version as read from version.txt: ${storm_tape_version}")
find_package(Git)
if (Git_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --dirty
    RESULT_VARIABLE result
    OUTPUT_VARIABLE describe
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(result)
    message("Failed to git describe: ${result}")
  else()
    message("git describe gives: ${describe}")
    string(FIND "${describe}" "${storm_tape_version}" position)
    if (${position} EQUAL 0)
      string(REPLACE "${storm_tape_version}" "" storm_tape_version_postfix ${describe})
    else()
      message(WARNING "the result of git describe is not compatible with the contents of version.txt")
      string(FIND "${describe}" "-" hyphen)
      string(SUBSTRING "${describe}" 0 ${hyphen} storm_tape_version)
      string(SUBSTRING "${describe}" ${hyphen} -1 storm_tape_version_postfix)
    endif()
  endif()
endif()

project(StoRM-Tape VERSION ${storm_tape_version})

message("StoRM-Tape base version: ${StoRM-Tape_VERSION}")

include(CTest)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(Boost_NO_WARN_NEW_VERSIONS 1)

# Add all warnings
string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wcast-qual -Wformat=2 -Wundef -Werror=float-equal -Wshadow -Wcast-align -Wunused -Wnull-dereference -Wdouble-promotion -Wimplicit-fallthrough -Wextra-semi -Woverloaded-virtual -Wnon-virtual-dtor -Wold-style-cast")

# Enable sanitizers
option(STORM_SANITIZE_ADDRESS   "Enable address sanitizer")
option(STORM_SANITIZE_UNDEFINED "Enable undefined-behaviour sanitizer")
option(STORM_SANITIZE_THREAD    "Enable thread sanitizer")

if (STORM_SANITIZE_ADDRESS)
  string(APPEND CMAKE_CXX_FLAGS " -fsanitize=address -fno-omit-frame-pointer")
  string(APPEND CMAKE_EXE_LINKER_FLAGS " -fsanitize=address -fno-omit-frame-pointer")
endif()

if (STORM_SANITIZE_UNDEFINED)
  string(APPEND CMAKE_CXX_FLAGS " -fsanitize=undefined -fno-omit-frame-pointer")
  string(APPEND CMAKE_EXE_LINKER_FLAGS " -fsanitize=undefined -fno-omit-frame-pointer")
endif()

if (STORM_SANITIZE_THREAD)
  # TODO: add compatibility check with other sanitizers
  string(APPEND CMAKE_CXX_FLAGS " -fsanitize=thread -fno-omit-frame-pointer")
  string(APPEND CMAKE_EXE_LINKER_FLAGS " -fsanitize=thread -fno-omit-frame-pointer")
endif()

# Create Coverage configuration
list(APPEND CMAKE_CONFIGURATION_TYPES Coverage)
set(CMAKE_CXX_FLAGS_COVERAGE "-g -O0 --coverage") 
set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "--coverage") 
set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE "--coverage")
set(CMAKE_MODULE_LINKER_FLAGS_COVERAGE "--coverage")

# Add "format-check" e "format-fix" executables for clang-format
include(cmake/lint-targets.cmake)

# https://github.com/open-telemetry/opentelemetry-cpp/blob/main/docs/building-with-stdlib.md
add_compile_definitions(OPENTELEMETRY_STL_VERSION=2020)

find_package(Crow CONFIG REQUIRED)
find_package(SOCI CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options url)
find_package(Fmt REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(opentelemetry-cpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)       # Required by otlp_http_client

add_library(
  libtaperestapi
  OBJECT
  src/access_logger.cpp
  src/archiveinfo_response.cpp
  src/cancel_response.cpp
  src/configuration.cpp
  src/database.cpp
  src/database_soci.cpp
  src/delete_response.cpp
  src/extended_attributes.cpp
  src/extended_file_status.cpp
  src/file.cpp
  src/http_text_map_carrier.cpp
  src/in_progress_response.cpp
  src/io.cpp
  src/json.cpp
  src/local_storage.cpp
  src/profiler.cpp
  src/release_response.cpp
  src/requests_with_paths.cpp
  src/routes.cpp
  src/stage_request.cpp
  src/stage_response.cpp
  src/status_response.cpp
  src/storage_area_resolver.cpp
  src/takeover_request.cpp
  src/tape_service.cpp
  src/telemetry.cpp
  src/telemetry_attributes.cpp
  src/trace_span.cpp
  src/tracer_provider.cpp
  src/types.cpp
  src/uuid_generator.cpp
)

target_link_libraries(
  libtaperestapi
  PUBLIC
  Crow::Crow 
  $<IF:$<TARGET_EXISTS:SOCI::soci_sqlite3>,SOCI::soci_sqlite3,SOCI::soci_sqlite3_static>
  $<IF:$<TARGET_EXISTS:SOCI::soci_core>,SOCI::soci_core,SOCI::soci_core_static>
  Boost::boost
  Boost::program_options
  Boost::url
  fmt::fmt
  yaml-cpp::yaml-cpp
  opentelemetry-cpp::api
  opentelemetry-cpp::sdk
  opentelemetry-cpp::otlp_http_exporter
)

add_executable(storm-tape src/main.cpp)

target_link_libraries(
  storm-tape 
  PRIVATE
  libtaperestapi 
)

if (BUILD_TESTING)
  add_subdirectory(tests)
endif()

set_target_properties(storm-tape PROPERTIES 
  DEBUG_POSTFIX "-debug"
  COVERAGE_POSTFIX "-cov"
)

include(GNUInstallDirs)
install(TARGETS storm-tape DESTINATION ${CMAKE_INSTALL_SBINDIR})

set(CPACK_PACKAGE_NAME storm-tape)
set(CPACK_PACKAGE_VENDOR INFN)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "StoRM Tape")
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_PACKAGE_VERSION "${storm_tape_version}${storm_tape_version_postfix}")
set(CPACK_RPM_PACKAGE_ARCHITECTURE x86_64)
set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_GENERATOR TGZ RPM)
set(CPACK_RPM_PACKAGE_RELEASE "1%{?dist}")
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
# https://stackoverflow.com/a/74348493/4377355
list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "/usr/sbin")
set(CPACK_RPM_DEBUGINFO_PACKAGE ON)
include(CPack)
