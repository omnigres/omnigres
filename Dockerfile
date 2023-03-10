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
ARG DEBIAN_VER=bullseye
# Version of Alpine Linux for PostgreSQL container
ARG DEBIAN_VER_PG=bullseye
# Build parallelism
ARG BUILD_PARALLEL_LEVEL

# Base builder image
FROM debian:${DEBIAN_VER}-slim AS builder
RUN echo "deb http://deb.debian.org/debian bullseye-backports main contrib non-free" >> /etc/apt/sources.list 
RUN apt update
RUN apt install -y wget build-essential git clang lld flex libreadline-dev zlib1g-dev libssl-dev tmux lldb gdb make perl
# current cmake is too old
ARG DEBIAN_VER
ENV DEBIAN_VER=${DEBIAN_VER}
RUN apt install -y cmake -t ${DEBIAN_VER}-backports
ARG USER
ARG UID
ARG PG
ARG BUILD_TYPE
ARG BUILD_PARALLEL_LEVEL
ENV USER=${USER}
ENV UID=$UID
ENV PG=${PG}
ENV BUILD_TYPE=${BUILD_TYPE}
ENV BUILD_PARALLEL_LEVEL=${BUILD_PARALLEL_LEVEL}
RUN mkdir -p /build /omni
RUN adduser --uid ${UID} ${USER} && chown -R ${USER} /build /omni
USER $USER
RUN git config --global --add safe.directory '*'

# PostgreSQL build
FROM builder AS postgres-build
COPY docker/CMakeLists.txt /omni/CMakeLists.txt
COPY cmake/FindPostgreSQL.cmake docker/PostgreSQLExtension.cmake /omni/cmake/
WORKDIR /build
RUN cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DPG=${PG} -DCMAKE_BUILD_PARALLEL_LEVEL=${BUILD_PARALLEL_LEVEL} /omni

# Omnigres build
FROM builder AS build
COPY --chown=${UID} . /omni
COPY --link --from=postgres-build --chown=${UID} /omni/.pg /omni/.pg
WORKDIR /build
RUN cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DPG=${PG} /omni
RUN make -j ${BUILD_PARALLEL_LEVEL} all
RUN make package

# Official PostgreSQL build
FROM postgres:${PG}-${DEBIAN_VER_PG} AS pg
COPY --from=build /build/packaged /omni
COPY docker/initdb/* /docker-entrypoint-initdb.d/
RUN cp -R /omni/extension $(pg_config --sharedir)/ && cp -R /omni/*.so $(pg_config --pkglibdir)/ && rm -rf /omni
RUN apt update && apt -y install libtclcl1 libpython3.9 libperl5.32
EXPOSE 8080
EXPOSE 5432
