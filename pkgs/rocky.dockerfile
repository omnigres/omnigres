# syntax=docker/dockerfile:1.5

ARG REDHAT_VER=9

FROM rockylinux:${REDHAT_VER} AS builder-rocky

ARG ARCH=x86_64
ARG POSTGRES_VER=16
ARG REDHAT_VER

# UPDATE
RUN dnf update -y

# SUDO IS USEFUL IF YOU WANT TO RUN AS YOUR OWN HOST'S USER & NOT ROOT
# E.G. `docker run -i -t --rm --user $(id -u):100 -v $PWD:/omni <image>`
# THIS WILL POP YOU INTO A CONTAINER WHERE YOU ARE 'redhat' IN THE GROUP
# 'users'. AND THE FILES YOU TOUCH/WRITE IN /omni WILL BE OWNED BY THE SAME
# USER ID AS YOUR HOST.
RUN dnf install -y sudo
RUN echo 'redhat ALL=(ALL) NOPASSWD: ALL' | tee /etc/sudoers.d/redhat
RUN useradd -g users -m redhat

# POSTGRES
RUN dnf install -y \
    https://download.postgresql.org/pub/repos/yum/reporpms/EL-${REDHAT_VER}-$(uname -m)/pgdg-redhat-repo-latest.noarch.rpm
RUN dnf --enablerepo=crb install -y \
    postgresql${POSTGRES_VER}-contrib \
    postgresql${POSTGRES_VER}-devel \
    postgresql${POSTGRES_VER}-plpython3 \
    postgresql${POSTGRES_VER}-server

# DEPS
RUN yum groupinstall -y "Development Tools"
RUN dnf --enablerepo=crb install -y \
    cmake \
    doxygen \
    nc \
    openssl-devel \
    perl-CPAN \
    python3-devel

# FPM
RUN dnf install -y ruby ruby-devel rubygems squashfs-tools
ENV GEM_HOME=/usr/local
RUN gem install fpm

ARG ITERATION=1

COPY ./ /omni
WORKDIR /build

# BUILD
ENV PATH /usr/pgsql-${POSTGRES_VER}/bin:${PATH}
RUN cmake -DPG_CONFIG=$(which pg_config) -DOPENSSL_CONFIGURED=1 /omni
RUN make -j all
RUN make package_extensions

WORKDIR /pkgs

# PACKAGING
RUN /omni/pkgs/mkrpms

# TEST INSTALL
RUN rpm -ivh *.rpm
