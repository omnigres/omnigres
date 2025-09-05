# syntax=docker/dockerfile:1.5
ARG OMNIGRES_REV=master
ARG PG_VERSION=17.6
ARG BASE=ghcr.io/cloudnative-pg/postgresql:${PG_VERSION}-standard-bookworm
ARG BUILD_PARALLEL_LEVEL=1

FROM ${BASE}
USER root
# Get necessary dependendcies
RUN apt-get -y update
RUN apt-get install -y wget clang-16 libc++-16-dev libc++abi-16-dev git cmake lld flex libreadline-dev zlib1g-dev libssl-dev tmux lldb gdb make perl python3-dev python3-venv python3-pip netcat-traditional bison
ARG PG_VERSION
ENV PG_VERSION=${PG_VERSION}
RUN apt-get install -y postgresql-plpython3-${PG_VERSION%.*} postgresql-server-dev-${PG_VERSION%.*}
# Use clang 16
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-16 100 && \
    update-alternatives --install /usr/bin/cc cc /usr/bin/clang-16 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-16 100 && \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-16 100
RUN apt-get remove -y gcc
# Clone Omnigres
ARG OMNIGRES_REV
ENV OMNIGRES_REV=${OMNIGRES_REV}
RUN git clone --recursive https://github.com/omnigres/omnigres /omnigres
RUN git -C /omnigres checkout ${OMNIGRES_REV}
# Build Omnigres
ARG BUILD_PARALLEL_LEVEL
ENV BUILD_PARALLEL_LEVEL=${BUILD_PARALLEL_LEVEL}
RUN cmake -S /omnigres -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPG_CONFIG=pg_config -DCMAKE_BUILD_PARALLEL_LEVEL=${BUILD_PARALLEL_LEVEL} -B /omnigres/build
RUN cmake --build /omnigres/build --parallel --target all --target install_extensions
# Symlink latest omni shared library
RUN <<EOF
export OMNI_SO_FILE="omni--$(grep "^omni=" /omnigres/versions.txt | cut -d "=" -f2).so"
ln -s $(pg_config --pkglibdir)/${OMNI_SO_FILE} $(pg_config --pkglibdir)/omni.so
unset OMNI_SO_FILE
EOF
# Cleanup
RUN rm -rf /omnigres
RUN apt-get remove -y wget clang-16 libc++-16-dev libc++abi-16-dev git cmake lld flex libreadline-dev zlib1g-dev libssl-dev lldb gdb make netcat-traditional bison
USER postgres
