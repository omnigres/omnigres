# syntax=docker/dockerfile:1.5

# Version of PostgreSQL
ARG PG=15.2
# Build type
ARG BUILD_TYPE=Release
# User name to be used for builder
ARG USER=omni
# Using 501 to match Colima's default
ARG UID=501
# Version of Alpine Linux for the builder
# Currently set to a snaposhot of `edge` to get the minimum requires version of `cmake` (3.25.1)
ARG ALPINE_VER=20230208
# Version of Alpine Linux for PostgreSQL container
ARG ALPINE_VER_PG=3.17

# Base builder image
FROM alpine:${ALPINE_VER} AS builder
RUN apk add git cmake clang lld flex readline-dev zlib-dev openssl-dev libssl3 openssl-libs-static libcrypto3 \
            tmux lldb gdb make musl-dev linux-headers perl
ARG USER
ARG UID
ARG PG
ARG BUILD_TYPE
ENV USER=${USER}
ENV UID=$UID
ENV PG=${PG}
ENV BUILD_TYPE=${BUILD_TYPE}
RUN mkdir -p /build /omni
RUN adduser -D --uid ${UID} ${USER} && chown -R ${USER} /build /omni
USER $USER
RUN git config --global --add safe.directory '*'

# PostgreSQL build
FROM builder AS postgres-build
COPY docker/CMakeLists.txt /omni/CMakeLists.txt
COPY cmake/FindPostgreSQL.cmake docker/PostgreSQLExtension.cmake /omni/cmake/
WORKDIR /build
RUN cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DPG=${PG} /omni

# Omnigres build
FROM builder AS build
COPY --chown=${UID} . /omni
COPY --link --from=postgres-build --chown=${UID} /omni/.pg /omni/.pg
WORKDIR /build
RUN cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DPG=${PG} /omni
RUN make -j all
RUN make package

# Official PostgreSQL build
FROM postgres:${PG}-alpine${ALPINE_VER_PG} AS pg
COPY --from=build /build/packaged /omni
COPY docker/initdb-omni.sql /docker-entrypoint-initdb.d/
RUN cp -R /omni/extension $(pg_config --sharedir)/ && cp -R /omni/*.so $(pg_config --pkglibdir)/ && rm -rf /omni
