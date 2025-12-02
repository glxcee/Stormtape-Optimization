# Telemetry
## Introduction

Telemetry is enabled by providing the corresponding configuration in
`storm-tape.conf`. Traces are created only if the `traceparent` header is
present in the incoming request.

This folder contains an example of NGINX reverse proxy able to split clients
with a certain percentage to enable/disable telemetry data.

## Configuration

Telemetry is enabled by providing the corresponding configuration in
`storm-tape.conf`. In order do enable telemetry, add the following section

```yaml
telemetry:
  service-name: storm-tape-example
  tracing-endpoint: "https://otello.cloud.cnaf.infn.it/collector/v1/traces"
```

### Docker network

To run this example it is necessary to create an external docker network in
order to connect the Dev Container and the demo instance of NGINX.

To create a new network, run

```bash
docker network create telemetry
```

then get you Dev Container container id with `docker ps`

```bash
docker ps 

CONTAINER ID   IMAGE                                                                             COMMAND                  CREATED       STATUS          PORTS                                     NAMES
09b7dd87cc72   vsc-storm-tape-b0ec7932e944a1274e5645308609b61a28544dc727550fdcc61bab7efbaee07d   "/bin/sh -c 'echo Co…"   3 weeks ago   Up 28 hours
```

and connect it to the newly create network

```bash
docker network connect telemetry 09b7dd87cc72
```

## Start the NGINX instance

```bash
cd telemetry
docker compose up -d
```

## Test the API

From *outside* the container (on your docker host/local machine) move to the
root project folder and run

```
curl -i -d @example/archive_info.json  http://localhost:8080/api/v1/archiveinfo
```

The port 8080 is mapped to the instrumented NGINX instance.

It is possibile to run [`scripts/simple_req_generator.sh`](../scripts/simple_req_generator.sh)
to generate a series of demo requests.
