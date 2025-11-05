# SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
#
# SPDX-License-Identifier: EUPL-1.2

FROM nginx:stable

RUN apt-get update && \
  apt-get install -y unzip curl gnupg2 ca-certificates lsb-release debian-archive-keyring && \
  curl https://nginx.org/keys/nginx_signing.key | gpg --dearmor | \
  tee /usr/share/keyrings/nginx-archive-keyring.gpg >/dev/null && \
  echo "deb [signed-by=/usr/share/keyrings/nginx-archive-keyring.gpg] \
  http://nginx.org/packages/debian `lsb_release -cs` nginx" | \
  tee /etc/apt/sources.list.d/nginx.list && \
  apt-get update && \
  apt-get install -y nginx-module-otel && \
  apt-get clean && \
  rm -rf /var/caches/apt
