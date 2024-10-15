AM_CPPFLAGS = \
        -I$(top_srcdir)/src \
        -I$(top_srcdir)/include
AM_CFLAGS =

lib_LTLIBRARIES = libfyaml.la
libfyaml_la_SOURCES = \
        lib/fy-parse.c lib/fy-parse.h \
	lib/fy-utf8.c lib/fy-utf8.h \
	lib/fy-types.c lib/fy-types.h \
	lib/fy-list.h \
	lib/fy-typelist.h \
	lib/fy-diag.c lib/fy-diag.h \
	lib/fy-dump.c lib/fy-dump.h \
	lib/fy-atom.c lib/fy-atom.h \
	lib/fy-ctype.c lib/fy-ctype.h \
	lib/fy-token.c lib/fy-token.h \
	lib/fy-input.c lib/fy-input.h \
	lib/fy-docstate.c lib/fy-docstate.h \
	lib/fy-doc.c lib/fy-doc.h \
	lib/fy-docbuilder.c lib/fy-docbuilder.h \
	lib/fy-emit.c lib/fy-emit.h lib/fy-emit-accum.h \
	lib/fy-utils.c lib/fy-utils.h \
	lib/fy-event.h lib/fy-event.c \
	xxhash/xxhash.c xxhash/xxhash.h \
	lib/fy-accel.c lib/fy-accel.h \
	lib/fy-walk.c lib/fy-walk.h \
	lib/fy-path.c lib/fy-path.h \
	lib/fy-composer.c lib/fy-composer.h

libfyaml_la_CPPFLAGS = $(AM_CPPFLAGS) \
				       -I$(top_srcdir)/src/lib \
				       -I$(top_srcdir)/src/xxhash
libfyaml_la_CFLAGS = $(AM_CFLAGS)
libfyaml_la_LDFLAGS = $(AM_LDFLAGS) $(AM_LIBLDFLAGS) \
		      -version $(LIBTOOL_VERSION)

bin_PROGRAMS =
noinst_PROGRAMS =

# libfyaml-parser needs both LIBYAML and static
if HAVE_LIBYAML
if HAVE_STATIC

noinst_PROGRAMS += libfyaml-parser

libfyaml_parser_SOURCES = \
	internal/libfyaml-parser.c \
	valgrind/fy-valgrind.h

libfyaml_parser_CPPFLAGS = $(AM_CPPFLAGS) \
			   -I$(top_srcdir)/src/valgrind \
			   -I$(top_srcdir)/src/lib \
			   -I$(top_srcdir)/src/xxhash
libfyaml_parser_LDADD = $(AM_LDADD) $(LIBYAML_LIBS) libfyaml.la
libfyaml_parser_CFLAGS = $(AM_CFLAGS) $(LIBYAML_CFLAGS)

libfyaml_parser_LDFLAGS = $(AM_LDFLAGS) -static
endif
endif

bin_PROGRAMS += fy-tool

fy_tool_SOURCES = \
	tool/fy-tool.c \
	valgrind/fy-valgrind.h

fy_tool_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/valgrind
fy_tool_LDADD = $(AM_LDADD) libfyaml.la
fy_tool_CFLAGS = $(AM_CFLAGS)
fy_tool_LDFLAGS = $(AM_LDFLAGS)

include_HEADERS = \
        $(top_srcdir)/include/libfyaml.h

install-exec-hook:
	(cd "$(DESTDIR)$(bindir)" && $(LN_S) -f fy-tool fy-dump)
	(cd "$(DESTDIR)$(bindir)" && $(LN_S) -f fy-tool fy-filter)
	(cd "$(DESTDIR)$(bindir)" && $(LN_S) -f fy-tool fy-testsuite)
	(cd "$(DESTDIR)$(bindir)" && $(LN_S) -f fy-tool fy-join)
	(cd "$(DESTDIR)$(bindir)" && $(LN_S) -f fy-tool fy-ypath)
	(cd "$(DESTDIR)$(bindir)" && $(LN_S) -f fy-tool fy-compose)

uninstall-hook:
	(cd "$(DESTDIR)$(bindir)" && rm -f fy-dump fy-filter fy-testsuite fy-join fy-ypath fy-compose)
