#!/bin/bash

# SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
#
# SPDX-License-Identifier: EUPL-1.2

set -ex

scripts_dir=$(dirname $(realpath "${0}"))
project_dir=$(dirname ${scripts_dir})
coverage_dir=${project_dir}/build/report

mkdir -p ${coverage_dir}

gcovr -v -f src -f tests \
  -e tests/doctest.h \
  -e src/main.cpp \
  -e src/profiler.cpp \
  -e src/routes.cpp \
  -e src/access_logger.cpp \
  --html-details ${coverage_dir}/index.html
