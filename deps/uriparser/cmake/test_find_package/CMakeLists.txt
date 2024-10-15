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
cmake_minimum_required(VERSION 3.0)

project(test-find-package VERSION 1.0)


# Cover version mismatch
find_package(uriparser 9999 CONFIG QUIET)
if (uriparser_FOUND)
    message(SEND_ERROR "Was not expecting for find uriparser 9999.x.x")
endif()


# Cover invalid required component
find_package(uriparser 0.9.1 CONFIG QUIET
    COMPONENTS
        invalid123
)
if (uriparser_FOUND)
    message(SEND_ERROR "Was not expecting for find uriparser with component \"invalid123\"")
endif()


# Cover invalid optional component
find_package(uriparser 0.9.1 CONFIG REQUIRED
    COMPONENTS
        char
        wchar_t
    OPTIONAL_COMPONENTS
        invalid123
)
if (NOT uriparser_FOUND)
    message(SEND_ERROR "Was expecting to find uriparser. Something is off.")
endif()


# Compile and link some hello world using uriparser
add_executable(hello
    hello.c
)

target_link_libraries(hello PUBLIC uriparser::uriparser)
