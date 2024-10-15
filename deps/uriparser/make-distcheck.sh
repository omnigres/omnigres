#! /usr/bin/env bash
# uriparser - RFC 3986 URI parsing library
#
# Copyright (C) 2018, Sebastian Pipping <sebastian@pipping.org>
# All rights reserved.
#
# Redistribution and use in source  and binary forms, with or without
# modification, are permitted provided  that the following conditions
# are met:
#
#     1. Redistributions  of  source  code   must  retain  the  above
#        copyright notice, this list  of conditions and the following
#        disclaimer.
#
#     2. Redistributions  in binary  form  must  reproduce the  above
#        copyright notice, this list  of conditions and the following
#        disclaimer  in  the  documentation  and/or  other  materials
#        provided with the distribution.
#
#     3. Neither the  name of the  copyright holder nor the  names of
#        its contributors may be used  to endorse or promote products
#        derived from  this software  without specific  prior written
#        permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND  ANY EXPRESS OR IMPLIED WARRANTIES,  INCLUDING, BUT NOT
# LIMITED TO,  THE IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS
# FOR  A  PARTICULAR  PURPOSE  ARE  DISCLAIMED.  IN  NO  EVENT  SHALL
# THE  COPYRIGHT HOLDER  OR CONTRIBUTORS  BE LIABLE  FOR ANY  DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL,  EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA,  OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT  LIABILITY,  OR  TORT (INCLUDING  NEGLIGENCE  OR  OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
#
set -e
set -u
PS4='# '

create_git_archive() (
    local tarname="$1"
    local format="$2"
    local git_archive_args=(
        --format="${format}"
        --prefix="${tarname}"/
        --output="${tarname}.${format}"
        "${@:3}"
    )
    set -x
    git archive "${git_archive_args[@]}" HEAD
)

compress_tarball() (
    local tarname="$1"
    local command="$2"
    local format="$3"
    echo "${PS4}${command} -c ${*:4} < ${tarname}.tar > ${tarname}.tar.${format}" >&2
    "${command}" -c "${@:4}" < "${tarname}".tar > "${tarname}.tar.${format}"
)

check_tarball() (
    local tarname="$1"
    set -x
    tar xf "${tarname}".tar
    (
        cd "${tarname}"

        mkdir build
        cd build

        cmake "${@:2}" ..
        make

        # NOTE: We need to copy some .dll files next to the
        #       Windows binaries so that they are ready to be executed
        if [[ "${*:2}" == *mingw* ]]; then
            cp /usr/lib/gcc/i686-w64-mingw32/*-posix/libgcc_s_sjlj-1.dll ./
            cp /usr/lib/gcc/i686-w64-mingw32/*-posix/libstdc++-6.dll ./
            cp /usr/i686-w64-mingw32/lib/libwinpthread-1.dll ./
            cp "${GTEST_PREFIX:?}"/bin/libgtest.dll ./
        fi

        make test
        make DESTDIR="${PWD}"/ROOT/ install
    )
    rm -Rf "${tarname}"
)

report() {
    local tarname="$1"
    echo =================================================
    echo "${tarname} archives ready for distribution:"
    ls -1 "${tarname}".*
    echo =================================================
}

cleanup() (
    local tarname="$1"
    set -x
    rm "${tarname}".tar
)

main() {
    local PACKAGE=uriparser
    local PACKAGE_VERSION="$(git describe | sed 's,^uriparser-,,')"
    local PACKAGE_TARNAME="${PACKAGE}-${PACKAGE_VERSION}"

    create_git_archive ${PACKAGE_TARNAME} tar
    check_tarball ${PACKAGE_TARNAME} "$@"

    create_git_archive ${PACKAGE_TARNAME} zip -9

    compress_tarball ${PACKAGE_TARNAME} bzip2 bz2 -9
    compress_tarball ${PACKAGE_TARNAME} gzip  gz  --best
    compress_tarball ${PACKAGE_TARNAME} lzip  lz  -9
    compress_tarball ${PACKAGE_TARNAME} xz    xz  -e

    cleanup ${PACKAGE_TARNAME}

    report ${PACKAGE_TARNAME}
}

main "$@"
