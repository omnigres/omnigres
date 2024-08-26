# syntax=docker/dockerfile:1.5

ARG UBUNTU_VER=noble

FROM ubuntu:${UBUNTU_VER} AS builder-ubuntu

ARG POSTGRES_VER=16
ARG UBUNTU_VER

# UPDATE
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -y

# SUDO IS USEFUL IF YOU WANT TO RUN AS YOUR OWN HOST'S USER & NOT ROOT
# E.G. `docker run -i -t --rm --user $(id -u):100 -v $PWD:/omni <image>`
# THIS WILL POP YOU INTO A CONTAINER WHERE YOU ARE 'ubuntu' IN THE GROUP
# 'users'. AND THE FILES YOU TOUCH/WRITE IN /omni WILL BE OWNED BY THE SAME
# USER ID AS YOUR HOST.
RUN apt-get install -y sudo
RUN echo 'ubuntu ALL=(ALL) NOPASSWD: ALL' | tee /etc/sudoers.d/ubuntu

# POSTGRES
RUN apt install -y postgresql-common
RUN yes|/usr/share/postgresql-common/pgdg/apt.postgresql.org.sh
RUN apt-get update
RUN apt-get install -y \
    postgresql-${POSTGRES_VER} \
    postgresql-contrib-${POSTGRES_VER} \
    postgresql-plpython3-${POSTGRES_VER} \
    postgresql-server-dev-${POSTGRES_VER}

# DEPS
RUN apt-get install -y -t ${UBUNTU_VER}-backports cmake
RUN apt-get install -y \
    build-essential \
    doxygen \
    flex \
    git \
    libreadline-dev \
    libssl-dev \
    ncat \
    pkg-config \
    python3-dev \
    python3-venv

# FPM
RUN apt-get install -y ruby ruby-dev ruby-rubygems squashfs-tools
ENV GEM_HOME=/usr/local
RUN gem install fpm

ARG ITERATION=1

COPY ./ /omni
WORKDIR /build

# BUILD
RUN cmake -DPG_CONFIG=$(which pg_config) -DOPENSSL_CONFIGURED=1 /omni
RUN make -j all
RUN make package_extensions

WORKDIR /pkgs

# PACKAGING
RUN /omni/pkgs/mkdebs

# TEST INSTALL
RUN dpkg -i *.deb
