FROM ubuntu:23.04

RUN apt update -y
RUN apt install -y git build-essential cmake flex libreadline-dev libz-dev libssl-dev
RUN apt install -y sudo rsync
# Using 501 to match Colima's default
RUN adduser --uid 501 omni
USER omni
RUN git config --global --add safe.directory '*'
ENV DOCKER_LINUX_BUILDER 1

VOLUME /omni
WORKDIR /omni