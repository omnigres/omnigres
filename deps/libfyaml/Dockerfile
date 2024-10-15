ARG IMAGE=ubuntu
FROM ${IMAGE}
# install build dependencies
RUN apt-get update -qq
RUN apt-get install --no-install-recommends -y \
      gcc autoconf automake libtool git make libyaml-dev libltdl-dev \
      pkg-config check python3 python3-pip python3-setuptools
# install sphinx doc dependencies
RUN pip3 install wheel sphinx git+http://github.com/return42/linuxdoc.git sphinx_rtd_theme sphinx-markdown-builder
# configure argument
ARG CONFIG_ARGS
ENV CONFIG_ARGS=${CONFIG_ARGS:-"--enable-debug --prefix=/usr"}
COPY . /build
WORKDIR /build
# do a maintainer clean if the directory was unclean (it can fail)
RUN make maintainer-clean >/dev/null 2>&1|| true
RUN ./bootstrap.sh 2>&1
RUN ./configure 2>&1 ${CONFIG_ARGS}
RUN make
RUN make check
RUN make distcheck
RUN make doc-html
