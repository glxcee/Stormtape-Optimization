# SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
#
# SPDX-License-Identifier: EUPL-1.2

FROM almalinux:9

ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=${USER_UID}

COPY base-scripts/*.sh /base-scripts/
COPY library-scripts/*.sh /library-scripts/

ENV VCPKG_ROOT=/vcpkg
ENV VCPKG_DOWNLOADS=/vcpkg_downloads
# Required when building for arch != x86. In any case, since we provide system
# ninja and cmake binaries, this flag works also for x86. See
# https://learn.microsoft.com/en-us/vcpkg/users/config-environment#vcpkg_force_system_binaries
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV VCPKG_DISABLE_METRICS=1

RUN chmod +x library-scripts/*.sh base-scripts/*.sh && \
  library-scripts/provide-deps.sh && \
  library-scripts/provide-user.sh ${USERNAME} ${USER_UID} ${USER_GID} && \
  base-scripts/install-vcpkg.sh ${USERNAME} && \
  base-scripts/install-gcovr.sh && \
  rm -rf library-scripts base-scripts

USER ${USERNAME}
