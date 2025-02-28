# syntax=docker/dockerfile:1.5

# Version of PostgreSQL
ARG PG=17
# Build type
ARG BUILD_TYPE=RelWithDebInfo
# User name to be used for builder
ARG USER=omni
# Using 501 to match Colima's default
ARG UID=501
# Version of Alpine Linux for the builder
# Currently set to a snaposhot of `edge` to get the minimum requires version of `cmake` (3.25.1)
ARG DEBIAN_VER=bookworm
# Version of Alpine Linux for PostgreSQL container
ARG DEBIAN_VER_PG=bookworm
# Build parallelism
ARG BUILD_PARALLEL_LEVEL
# plrust version
ARG PLRUST_VERSION=1.2.8

# Base builder image
FROM debian:${DEBIAN_VER}-slim AS builder
RUN echo "deb http://deb.debian.org/debian bookworm-backports main contrib non-free" >> /etc/apt/sources.list
RUN apt-get update
RUN apt-get install -y wget build-essential git clang lld flex libreadline-dev zlib1g-dev libssl-dev tmux lldb gdb make perl python3-dev python3-venv python3-pip netcat-traditional bison
# current cmake is too old
ARG DEBIAN_VER
ENV DEBIAN_VER=${DEBIAN_VER}
RUN apt-get install -y cmake -t ${DEBIAN_VER}-backports
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
COPY cmake/FindPostgreSQL.cmake cmake/OpenSSL.cmake cmake/CPM.cmake docker/PostgreSQLExtension.cmake /omni/cmake/
WORKDIR /build
RUN cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DPGVER=${PG} -DCMAKE_BUILD_PARALLEL_LEVEL=${BUILD_PARALLEL_LEVEL} /omni

# Omnigres build
FROM builder AS build
COPY --chown=${UID} . /omni
COPY --link --from=postgres-build --chown=${UID} /omni/.pg /omni/.pg
WORKDIR /build
RUN cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DPGVER=${PG} /omni
RUN make -j ${BUILD_PARALLEL_LEVEL} all
RUN make package_extensions

FROM squidfunk/mkdocs-material AS docs
COPY --chown=${UID} . /docs
RUN pip install -r docs/requirements.txt
RUN mkdocs build -d /output
ENTRYPOINT ["/bin/sh"]

# plrust build
#FROM postgres:${PG}-${DEBIAN_VER_PG}  AS plrust
#ARG PLRUST_VERSION
#ENV PLRUST_VERSION=${PLRUST_VERSION}
#ARG PG
#ENV PG=${PG}
#RUN apt-get update && apt-get install -y curl pkg-config git build-essential libssl-dev libclang-dev flex libreadline-dev zlib1g-dev crossbuild-essential-arm64 crossbuild-essential-amd64
#USER postgres
#RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
#RUN . "$HOME/.cargo/env" && \
#    rustup default 1.72.0 && rustup component add llvm-tools-preview rustc-dev && \
#    rustup target install x86_64-unknown-linux-gnu && \
#    rustup target install aarch64-unknown-linux-gnu
#WORKDIR /var/lib/postgresql
#RUN git clone https://github.com/tcdi/plrust.git plrust && cd plrust && git checkout v${PLRUST_VERSION}
#RUN . "$HOME/.cargo/env" && cd plrust/plrustc && ./build.sh && mv ../build/bin/plrustc ~/.cargo/bin && cd ..
#RUN . "$HOME/.cargo/env" && cd plrust/plrust && cargo install cargo-pgrx --version 0.12.5 --locked
#USER root
#RUN PG_VER=${PG%.*} && apt-get install -y postgresql-server-dev-${PG_VER}
#RUN chown -R postgres /usr/share/postgresql /usr/lib/postgresql
#USER postgres
#RUN PG_VER=${PG%.*} && . "$HOME/.cargo/env" && cargo pgrx init --pg${PG_VER} /usr/bin/pg_config
#RUN export USER=postgres && PG_VER=${PG%.*} && . "$HOME/.cargo/env" && cd plrust/plrust && ./build && cd ..
#RUN PG_VER=${PG%.*} && . "$HOME/.cargo/env" && cd plrust/plrust && \
#    cargo pgrx package --features "pg${PG_VER} trusted"

FROM ghcr.io/omnigres/omnigres-${PG} AS previous
ENV PG=${PG}
RUN mkdir -p /prev/libdir /prev/extension && \
    cp -R $(pg_config --pkglibdir)/* /prev/libdir/ && \
    cp -R $(pg_config --sharedir)/extension/* /prev/extension/

# Official slim PostgreSQL build
FROM postgres:${PG}-${DEBIAN_VER_PG} AS pg-slim
ARG PG
ENV PG=${PG}
ENV POSTGRES_DB=omnigres
ENV POSTGRES_USER=omnigres
ENV POSTGRES_PASSWORD=omnigres
RUN apt-get update && apt-get -y install libtclcl1 libpython3.11 libperl5.36 gettext
RUN PG_VER=${PG%.*} && apt-get update && apt-get -y install postgresql-pltcl-${PG_VER} postgresql-plperl-${PG_VER} postgresql-plpython3-${PG_VER} python3-dev python3-venv python3-pip
RUN apt-get -y install curl
RUN curl -fsSL https://pkgs.tailscale.com/stable/debian/bookworm.noarmor.gpg | tee /usr/share/keyrings/tailscale-archive-keyring.gpg >/dev/null && \
    curl -fsSL https://pkgs.tailscale.com/stable/debian/bookworm.tailscale-keyring.list | tee /etc/apt/sources.list.d/tailscale.list
RUN apt-get update && apt-get -y install tailscale
RUN apt-get install -y lldb-16 libc6-dbg vim valgrind # Ultimate debug toolkit
COPY docker/entrypoint.sh /usr/local/bin/omnigres-entrypoint.sh
RUN curl -fsSL https://repo.pigsty.io/key | gpg --dearmor -o /etc/apt/keyrings/pigsty.gpg
COPY docker/pigsty-io.list /etc/apt/sources.list.d/pigsty-io.list
RUN apt-get update
# Install the "must have" extension. Subjective.
RUN apt-get -y install \
    postgresql-${PG%.*}-plpgsql-check \
    postgresql-${PG%.*}-cron \
    postgresql-${PG%.*}-repack \
    postgresql-${PG%.*}-wal2json \
    postgresql-${PG%.*}-pgvector postgresql-${PG%.*}-pgvectorscale
COPY --from=previous /prev /prev
RUN mv -n /prev/libdir/* $(pg_config --pkglibdir)/ && \
    mv -n /prev/extension/* $(pg_config --sharedir)/extension/ && \
    rm -rf /prev
COPY --from=build /build/packaged /omni
# need versions.txt to pick versions
COPY --from=build /omni/versions.txt /omni/versions.txt
COPY --from=build /build/python-index /python-packages
COPY --from=build /build/python-wheels /python-wheels
RUN mkdir -p $(pg_config --pkglibdir)/omni_python--$(grep "^omni_python=" /omni/versions.txt | cut -d "=" -f2)
RUN mv /python-wheels/* $(pg_config --pkglibdir)/omni_python--$(grep "^omni_python=" /omni/versions.txt | cut -d "=" -f2)
# clear it in case it already exists
RUN rm -rf /docker-entrypoint-initdb.d
# copy template files, substituted later
COPY docker/initdb-slim/* /docker-entrypoint-initdb.d/
RUN cp -R /omni/extension $(pg_config --sharedir)/ && cp -R /omni/*.so $(pg_config --pkglibdir)/
# replace env variables in init scripts using envsubst
RUN <<EOF
# export variables used in template files
export OMNI_SO_FILE="omni--$(grep "^omni=" /omni/versions.txt | cut -d "=" -f2).so"
for file in $(ls /docker-entrypoint-initdb.d); do
  # only substitute exported variables in template file to avoid accidental substitution
  envsubst '$OMNI_SO_FILE' < /docker-entrypoint-initdb.d/$file > /docker-entrypoint-initdb.d/$(basename $file .templ)
  # remove template file
  rm /docker-entrypoint-initdb.d/$file
done
# unset exported variables after substitution
unset OMNI_SO_FILE
EOF
RUN rm -rf /omni
COPY --from=docs /output /omni-docs
ENTRYPOINT ["omnigres-entrypoint.sh"]
CMD ["postgres"]
EXPOSE 22
EXPOSE 8080
EXPOSE 5432

# Official PostgreSQL build
FROM pg-slim AS pg
ENV PG=${PG}
COPY docker/apt-pin /etc/apt/preferences.d/99-pin
RUN apt-get -y install $(apt-cache search  "^postgresql-${PG%.*}-*" | cut -d' ' -f1 | grep -v 'omnigres' | grep -v 'pg-duckdb' | grep -v 'hunspell' | grep -v 'citus' | grep -v 'pgdg' | grep -v 'timescaledb-tsl' | grep -v 'anonymizer' | grep -v 'dbgsym' | grep -v "${PG%.*}-zstd" | grep -v "${PG%.*}-tableversion" | grep -v "-documentdb-core")
#COPY --from=plrust /var/lib/postgresql/plrust/target/release /plrust-release
## clear it in case it already exists
#RUN rm -rf /docker-entrypoint-initdb.d
## copy template files, substituted later
#COPY docker/initdb/* /docker-entrypoint-initdb.d/
## need versions.txt to pick omni version
#COPY --from=build /omni/versions.txt /omni/versions.txt
## replace env variables in init scripts using envsubst
#RUN <<EOF
## export variables used in template files
#export OMNI_SO_FILE="omni--$(grep "^omni=" /omni/versions.txt | cut -d "=" -f2).so"
#for file in $(ls /docker-entrypoint-initdb.d); do
#  # only substitute exported variables in template file to avoid accidental substitution
#  envsubst '$OMNI_SO_FILE' < /docker-entrypoint-initdb.d/$file > /docker-entrypoint-initdb.d/$(basename $file .templ)
#  # remove template file
#  rm /docker-entrypoint-initdb.d/$file
#done
## unset exported variables after substitution
#unset OMNI_SO_FILE
#EOF
#RUN PG_VER=${PG%.*} && cp /plrust-release/plrust-pg${PG_VER}/usr/lib/postgresql/${PG_VER}/lib/plrust.so $(pg_config --pkglibdir) && \
#    cp /plrust-release/plrust-pg${PG_VER}/usr/share/postgresql/${PG_VER}/extension/plrust* $(pg_config --sharedir)/extension && \
#    rm -rf /plrust-release && rm -rf /omni
#RUN apt-get -y install libclang-dev build-essential crossbuild-essential-arm64 crossbuild-essential-amd64
#COPY --from=plrust /var/lib/postgresql/.cargo /var/lib/postgresql/.cargo
#COPY --from=plrust /var/lib/postgresql/.rustup /var/lib/postgresql/.rustup
#ENV PATH="/var/lib/postgresql/.cargo/bin:$PATH"
#RUN PG_VER=${PG%.*} && apt-get install -y postgresql-server-dev-${PG_VER}
#USER postgres
#RUN PG_VER=${PG%.*} && rustup default 1.72.0 && cargo pgrx init --pg${PG_VER} /usr/bin/pg_config
## Prime the compiler so it doesn't take forever on first compilation
#WORKDIR /var/lib/postgresql
#RUN initdb -D prime &&  pg_ctl start -o "-c shared_preload_libraries='plrust' -c plrust.work_dir='/tmp' -c listen_addresses='' " -D prime && \
#    createdb -h /var/run/postgresql prime && \
#    psql -h /var/run/postgresql prime -c "create extension plrust; create function test () returns bool language plrust as 'Ok(Some(true))';" && \
#    pg_ctl stop -D prime && rm -rf prime
#USER root

# Omnikube build
FROM postgres:${PG}-${DEBIAN_VER_PG} AS omnikube
ARG PG
ENV PG=${PG}
ENV POSTGRES_DB=omnikube
ENV POSTGRES_USER=omnikube
ENV POSTGRES_PASSWORD=omnikube
RUN apt-get update && apt-get -y install libtclcl1 libpython3.11 libperl5.36 gettext
RUN PG_VER=${PG%.*} && apt-get update && apt-get -y install postgresql-pltcl-${PG_VER} postgresql-plperl-${PG_VER} postgresql-plpython3-${PG_VER} python3-dev python3-venv python3-pip
RUN apt-get -y install curl
COPY docker/entrypoint.sh /usr/local/bin/omnigres-entrypoint.sh
COPY --from=build /build/packaged /omni
COPY --from=build /build/python-index /python-packages
COPY --from=build /build/python-wheels /python-wheels
# clear it in case it already exists
RUN rm -rf /docker-entrypoint-initdb.d
# copy template files, substituted later
COPY docker/initdb-omnikube/* /docker-entrypoint-initdb.d/
RUN cp -R /omni/extension $(pg_config --sharedir)/ && cp -R /omni/*.so $(pg_config --pkglibdir)/
# need versions.txt to pick omni version
COPY --from=build /omni/versions.txt /omni/versions.txt
# replace env variables in init scripts using envsubst
RUN <<EOF
# export variables used in template files
export OMNI_SO_FILE="omni--$(grep "^omni=" /omni/versions.txt | cut -d "=" -f2).so"
for file in $(ls /docker-entrypoint-initdb.d); do
  # only substitute exported variables in template file to avoid accidental substitution
  envsubst '$OMNI_SO_FILE' < /docker-entrypoint-initdb.d/$file > /docker-entrypoint-initdb.d/$(basename $file .templ)
  # remove template file
  rm /docker-entrypoint-initdb.d/$file
done
# unset exported variables after substitution
unset OMNI_SO_FILE
EOF
RUN rm -rf /omni
ENTRYPOINT ["omnigres-entrypoint.sh"]
CMD ["postgres"]
EXPOSE 22
EXPOSE 5432
EXPOSE 8081
