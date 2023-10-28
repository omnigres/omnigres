# syntax=docker/dockerfile:1.5

# Version of PostgreSQL
ARG PG=16.0
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
# plrust version
ARG PLRUST_VERSION=1.2.6

# Base builder image
FROM debian:${DEBIAN_VER}-slim AS builder
RUN echo "deb http://deb.debian.org/debian bullseye-backports main contrib non-free" >> /etc/apt/sources.list 
RUN apt update
RUN apt install -y wget build-essential git clang lld flex libreadline-dev zlib1g-dev libssl-dev tmux lldb gdb make perl python3-dev python3-venv python3-pip netcat
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
COPY cmake/FindPostgreSQL.cmake cmake/OpenSSL.cmake docker/PostgreSQLExtension.cmake /omni/cmake/
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

# plrust build
FROM postgres:${PG}-${DEBIAN_VER_PG}  AS plrust
ARG PLRUST_VERSION
ENV PLRUST_VERSION=${PLRUST_VERSION}
ARG PG
ENV PG=${PG}
RUN apt update && apt install -y curl pkg-config git build-essential libssl-dev libclang-dev flex libreadline-dev zlib1g-dev crossbuild-essential-arm64 crossbuild-essential-amd64
USER postgres
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
RUN . "$HOME/.cargo/env" && \
    rustup default 1.72.0 && rustup component add llvm-tools-preview rustc-dev && \
    rustup target install x86_64-unknown-linux-gnu && \
    rustup target install aarch64-unknown-linux-gnu
WORKDIR /var/lib/postgresql
RUN git clone https://github.com/tcdi/plrust.git plrust && cd plrust && git checkout v${PLRUST_VERSION}
RUN . "$HOME/.cargo/env" && cd plrust/plrustc && ./build.sh && mv ../build/bin/plrustc ~/.cargo/bin && cd ..
RUN . "$HOME/.cargo/env" && cd plrust/plrust && cargo install cargo-pgrx --locked
USER root
RUN PG_VER=${PG%.*} && apt install -y postgresql-server-dev-${PG_VER}
RUN chown -R postgres /usr/share/postgresql /usr/lib/postgresql
USER postgres
RUN PG_VER=${PG%.*} && . "$HOME/.cargo/env" && cargo pgrx init --pg${PG_VER} /usr/bin/pg_config
RUN export USER=postgres && PG_VER=${PG%.*} && . "$HOME/.cargo/env" && cd plrust/plrust && ./build && cd ..
RUN PG_VER=${PG%.*} && . "$HOME/.cargo/env" && cd plrust/plrust && \
    cargo pgrx package --features "pg${PG_VER} trusted"

# Official slim PostgreSQL build
FROM postgres:${PG}-${DEBIAN_VER_PG} AS pg-slim
ARG PG
ENV PG=${PG}
ENV POSTGRES_DB=omnigres
ENV POSTGRES_USER=omnigres
ENV POSTGRES_PASSWORD=omnigres
COPY --from=build /build/packaged /omni
COPY --from=build /build/python-index /python-packages
COPY --from=build /build/python-wheels /python-wheels
COPY docker/initdb-slim/* /docker-entrypoint-initdb.d/
RUN cp -R /omni/extension $(pg_config --sharedir)/ && cp -R /omni/*.so $(pg_config --pkglibdir)/ && rm -rf /omni
RUN apt update && apt -y install libtclcl1 libpython3.9 libperl5.32
RUN PG_VER=${PG%.*} && apt update && apt -y install postgresql-pltcl-${PG_VER} postgresql-plperl-${PG_VER} postgresql-plpython3-${PG_VER} python3-dev python3-venv python3-pip
EXPOSE 8080
EXPOSE 5432

# Official PostgreSQL build
FROM pg-slim AS pg
COPY --from=plrust /var/lib/postgresql/plrust/target/release /plrust-release
COPY docker/initdb/* /docker-entrypoint-initdb.d/
RUN PG_VER=${PG%.*} && cp /plrust-release/plrust-pg${PG_VER}/usr/lib/postgresql/${PG_VER}/lib/plrust.so $(pg_config --pkglibdir) && \
    cp /plrust-release/plrust-pg${PG_VER}/usr/share/postgresql/${PG_VER}/extension/plrust* $(pg_config --sharedir)/extension && \
    rm -rf /plrust-release
RUN apt -y install libclang-dev build-essential crossbuild-essential-arm64 crossbuild-essential-amd64
COPY --from=plrust /var/lib/postgresql/.cargo /var/lib/postgresql/.cargo
COPY --from=plrust /var/lib/postgresql/.rustup /var/lib/postgresql/.rustup
ENV PATH="/var/lib/postgresql/.cargo/bin:$PATH"
RUN PG_VER=${PG%.*} && apt install -y postgresql-server-dev-${PG_VER}
USER postgres
RUN PG_VER=${PG%.*} && rustup default 1.72.0 && cargo pgrx init --pg${PG_VER} /usr/bin/pg_config
# Prime the compiler so it doesn't take forever on first compilation
WORKDIR /var/lib/postgresql
RUN initdb -D prime &&  pg_ctl start -o "-c shared_preload_libraries='plrust' -c plrust.work_dir='/tmp' -c listen_addresses='' " -D prime && \
    createdb -h /var/run/postgresql prime && \
    psql -h /var/run/postgresql prime -c "create extension plrust; create function test () returns bool language plrust as 'Ok(Some(true))';" && \
    pg_ctl stop -D prime && rm -rf prime
USER root
