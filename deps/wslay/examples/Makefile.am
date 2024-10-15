# Wslay - The WebSocket Library

# Copyright (c) 2011, 2013 Tatsuhiro Tsujikawa

# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:

# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

if ENABLE_EXAMPLES

AM_CFLAGS = $(WARNCFLAGS)
AM_CXXFLAGS = $(WARNCXXFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/lib/includes \
	-I$(top_builddir)/lib/includes @NETTLE_CFLAGS@ @DEFS@
AM_LDFLAGS = @NETTLE_LIBS@
LDADD = $(top_builddir)/lib/libwslay.la

bin_PROGRAMS = fork-echoserv echoserv testclient

fork_echoserv_SOURCES = fork-echoserv.c
echoserv_SOURCES = echoserv.cc
testclient_SOURCES = testclient.cc

endif # ENABLE_EXAMPLES
