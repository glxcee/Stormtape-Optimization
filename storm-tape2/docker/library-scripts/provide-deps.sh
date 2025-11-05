#!/bin/bash

# SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
#
# SPDX-License-Identifier: EUPL-1.2

STORM_BUILD_TYPE=${1}

if [[ $STORM_BUILD_TYPE == "debug" ]] 
then 
    echo "Installing depencencies for debug"
    dnf install -y \
        libubsan \
        libasan
else
    echo "Skipping debug dependencies"
fi
