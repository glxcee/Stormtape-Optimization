#!/bin/bash

# SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
#
# SPDX-License-Identifier: EUPL-1.2

set -ex

storm_endpoint=${1:-"http://localhost:8080"}

stage_id=$(curl -s -d @example/stage_request.json ${storm_endpoint}/stage | jq -r .requestId)
echo "stage id: ${stage_id}"
curl -i ${storm_endpoint}/stage/${stage_id}
curl -i -d @example/archive_info.json ${storm_endpoint}/archiveinfo
curl -i -d @example/cancel_request.json ${storm_endpoint}/stage/${stage_id}/cancel
curl -i -X DELETE ${storm_endpoint}/stage/${stage_id}
curl -i -X GET ${storm_endpoint}/stage/123
curl -i -X DELETE ${storm_endpoint}/stage/123
