# SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
#
# SPDX-License-Identifier: EUPL-1.2

FROM almalinux:9

ARG USERNAME=storm
ARG USER_UID=1000
ARG USER_GID=${USER_UID}
ARG WORK_DIR=storm-tape

COPY ./library-scripts/*.sh /library-scripts/
COPY ./storm-tape-*.rpm /

RUN chmod +x library-scripts/*.sh && \
    library-scripts/provide-deps.sh && \
    library-scripts/provide-user.sh ${USERNAME} ${USER_UID} ${USER_GID} && \
    mkdir ${WORK_DIR} && \
    chown -R ${USERNAME}: ${WORK_DIR} && \
    rm -rf library-scripts && \
    rpm -i storm-tape-*.rpm && \
    rm -rf storm-tape-*.rpm

USER ${USERNAME}
WORKDIR ${WORK_DIR}
