#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
#
# SPDX-License-Identifier: EUPL-1.2

set -ex

# Enable CRB for ninja
dnf -y install yum-utils
dnf config-manager --set-enabled crb
dnf update -y

dnf install -y --setopt=tsflags=nodocs \
    sudo \
    git \
    wget \
    zip \
    unzip \
    ninja-build \
    cmake \
    rpm-build \
    gcc-c++ \
    libubsan \
    libasan \
    which \
    python3-pip \
    perl

dnf clean all
rm -rf /var/cache/dnf
