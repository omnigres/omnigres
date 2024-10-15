# test-suite (run with 'make check')

AM_CPPFLAGS = \
        -I$(top_srcdir)/include
AM_CFLAGS =

AM_TESTS_ENVIRONMENT= \
	TOP_SRCDIR="${top_srcdir}" \
	TOP_BUILDDIR="${top_builddir}" \
	SRCDIR="${srcdir}" \
	BUILDDIR="${builddir}" \
	JQ="@JQ@"

TESTS_ENVIRONMENT= \
	TOP_SRCDIR="${top_srcdir}" \
	TOP_BUILDDIR="${top_builddir}" \
	SRCDIR="${srcdir}" \
	BUILDDIR="${builddir}" \
	JQ="@JQ@"

# TEST_EXTENSIONS = .test

TEST_LOG_COMPILE = $(SHELL)
TEST_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/build-aux/tap-driver.sh

TESTS =

if HAVE_COMPATIBLE_CHECK

check_PROGRAMS = libfyaml-test
libfyaml_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/valgrind/ \
			 -I$(top_srcdir)/src/lib/
libfyaml_test_LDADD = $(AM_LDADD) $(CHECK_LIBS) $(top_builddir)/src/libfyaml.la
libfyaml_test_CFLAGS = $(AM_CFLAGS) $(CHECK_CFLAGS) 
libfyaml_test_LDFLAGS = $(AM_LDFLAGS) $(CHECK_LDFLAGS)

libfyaml_test_SOURCES = \
	libfyaml-test.c \
	libfyaml-test-core.c \
	libfyaml-test-meta.c \
	libfyaml-test-emit.c

# the private tests require static compilation
# if we're compiling shared only, disable the private parts
if HAVE_STATIC
libfyaml_test_SOURCES += libfyaml-test-private.c
libfyaml_test_LDFLAGS += -static
endif

TESTS += libfyaml.test

endif

if HAVE_NETWORK
if HAVE_GIT

# normal YAML testsuite
TESTS += testsuite.test

# YAML testsuite JSON generation
if HAVE_JQ
TESTS += testsuite-json.test
endif

# normal JSON testsuite
TESTS += jsontestsuite.test

# normal YAML testsuite using document event stream
TESTS += testsuite-evstream.test

testsuite.test: test-suite-data json-test-suite-data

test-suite-data:
	@GIT@ clone "@TESTSUITEURL@" -- $@ && \
	cd $@ && \
	@GIT@ checkout --detach @TESTSUITECHECKOUT@

jsontestsuite.test: json-test-suite-data

json-test-suite-data:
	@GIT@ clone "@JSONTESTSUITEURL@" -- $@ && \
	cd $@ && \
	@GIT@ checkout --detach @JSONTESTSUITECHECKOUT@

check-local: test-suite-data json-test-suite-data

distclean-local:
	@rm -rf test-suite-data json-test-suite-data

endif
endif

TESTS += testerrors.test

TESTS += testemitter.test
TESTS += testemitter-streaming.test

EXTRA_DIST = test-env $(TESTS)

# Add the error test files
EXTRA_DIST += \
	test-errors/0002/=== \
	test-errors/0002/in.yaml \
	test-errors/0002/test.error \
	test-errors/0003/=== \
	test-errors/0003/in.yaml \
	test-errors/0003/test.error \
	test-errors/0004/=== \
	test-errors/0004/in.yaml \
	test-errors/0004/test.error \
	test-errors/0005/=== \
	test-errors/0005/in.yaml \
	test-errors/0005/test.error \
	test-errors/0006/=== \
	test-errors/0006/in.yaml \
	test-errors/0006/test.error \
	test-errors/0007/=== \
	test-errors/0007/in.yaml \
	test-errors/0007/test.error \
	test-errors/0008/=== \
	test-errors/0008/in.yaml \
	test-errors/0008/test.error \
	test-errors/0009/=== \
	test-errors/0009/in.yaml \
	test-errors/0009/test.error \
	test-errors/0010/=== \
	test-errors/0010/in.yaml \
	test-errors/0010/test.error \
	test-errors/0011/=== \
	test-errors/0011/in.yaml \
	test-errors/0011/test.error

# Add the emitter test files
EXTRA_DIST += \
	emitter-examples/anchors-1.yaml \
	emitter-examples/anchors-2.yaml \
	emitter-examples/anchors-3.yaml \
	emitter-examples/anchors-4.1.yaml \
	emitter-examples/anchors-4.yaml \
	emitter-examples/anchors-on-empty-scalars1.yaml \
	emitter-examples/anchors-on-empty-scalars2.yaml \
	emitter-examples/anchors-on-empty-scalars3.yaml \
	emitter-examples/anchors-on-empty-scalars4.yaml \
	emitter-examples/anchors-on-empty-scalars.yaml \
	emitter-examples/anchors.yaml \
	emitter-examples/array.yaml \
	emitter-examples/block2.yaml \
	emitter-examples/block3.yaml \
	emitter-examples/block4.yaml \
	emitter-examples/block6.yaml \
	emitter-examples/block7.yaml \
	emitter-examples/blocked.yaml \
	emitter-examples/blockind.yaml \
	emitter-examples/block.yaml \
	emitter-examples/c10.yaml \
	emitter-examples/c11.yaml \
	emitter-examples/c12.yaml \
	emitter-examples/c13.yaml \
	emitter-examples/c1.yaml \
	emitter-examples/c2.yaml \
	emitter-examples/c3.yaml \
	emitter-examples/c4.yaml \
	emitter-examples/c5.yaml \
	emitter-examples/c6.yaml \
	emitter-examples/c7.yaml \
	emitter-examples/c8.yaml \
	emitter-examples/c9.yaml \
	emitter-examples/compact1.yaml \
	emitter-examples/compactblockmap.yaml \
	emitter-examples/complexkey2.yaml \
	emitter-examples/complexkey3.yaml \
	emitter-examples/complexkey4.yaml \
	emitter-examples/complexkey5.yaml \
	emitter-examples/complexkey6.yaml \
	emitter-examples/complexkey7.yaml \
	emitter-examples/complexkey8.yaml \
	emitter-examples/complexkey9.yaml \
	emitter-examples/complexkey.yaml \
	emitter-examples/docstartend.yaml \
	emitter-examples/dqscalar.yaml \
	emitter-examples/dqzero.yaml \
	emitter-examples/emoji.yaml \
	emitter-examples/emptydoc.yaml \
	emitter-examples/emptykey.yaml \
	emitter-examples/emptystream.yaml \
	emitter-examples/flow1.yaml \
	emitter-examples/flow2.yaml \
	emitter-examples/flow.yaml \
	emitter-examples/fold2.yaml \
	emitter-examples/fold3.yaml \
	emitter-examples/fold4.yaml \
	emitter-examples/fold5.yaml \
	emitter-examples/folded2.yaml \
	emitter-examples/folded.yaml \
	emitter-examples/folding.yaml \
	emitter-examples/fold.yaml \
	emitter-examples/global-tag.yaml \
	emitter-examples/invoice.yaml \
	emitter-examples/json.yaml \
	emitter-examples/keyflow.yaml \
	emitter-examples/keykey2.yaml \
	emitter-examples/keykey.yaml \
	emitter-examples/line.yaml \
	emitter-examples/literal1.yaml \
	emitter-examples/literal2.yaml \
	emitter-examples/literal3.yaml \
	emitter-examples/literal4.yaml \
	emitter-examples/literal.yaml \
	emitter-examples/mapping.yaml \
	emitter-examples/mergekeyspec.yaml \
	emitter-examples/multi-document.yaml \
	emitter-examples/multiline-quoted-key.yaml \
	emitter-examples/multiline-simple-key.yaml \
	emitter-examples/nodeprop2.yaml \
	emitter-examples/nodeprop.yaml \
	emitter-examples/numbers-flow.yaml \
	emitter-examples/numbers.yaml \
	emitter-examples/plainlines.yaml \
	emitter-examples/plain-scalars-with-commas.yaml \
	emitter-examples/plainscalar.yaml \
	emitter-examples/quotedbackslash.yaml \
	emitter-examples/quoted.yaml \
	emitter-examples/scalar-multiline.yaml \
	emitter-examples/scalars2.yaml \
	emitter-examples/scalar-singlequoted.yaml \
	emitter-examples/scalar-space1.yaml \
	emitter-examples/scalar-space.yaml \
	emitter-examples/scalars.yaml \
	emitter-examples/scanner-c-10.yaml \
	emitter-examples/scanner-c-11.yaml \
	emitter-examples/scanner-c-12.yaml \
	emitter-examples/scanner-c-13.yaml \
	emitter-examples/scanner-c-1.yaml \
	emitter-examples/scanner-c-2.yaml \
	emitter-examples/scanner-c-3.yaml \
	emitter-examples/scanner-c-4.yaml \
	emitter-examples/scanner-c-5.yaml \
	emitter-examples/scanner-c-6.yaml \
	emitter-examples/scanner-c-7.yaml \
	emitter-examples/scanner-c-8-2.yaml \
	emitter-examples/scanner-c-8.yaml \
	emitter-examples/scanner-c-9.yaml \
	emitter-examples/seq1.yaml \
	emitter-examples/seq2.yaml \
	emitter-examples/seq3.yaml \
	emitter-examples/seq4.yaml \
	emitter-examples/seq5.yaml \
	emitter-examples/seq6.yaml \
	emitter-examples/seq.yaml \
	emitter-examples/sets.yaml \
	emitter-examples/simple1.yaml \
	emitter-examples/simple2.yaml \
	emitter-examples/simpleanchor1.yaml \
	emitter-examples/simpleanchor2.yaml \
	emitter-examples/simpleanchor3.yaml \
	emitter-examples/simpleanchor4.yaml \
	emitter-examples/simpleanchor.yaml \
	emitter-examples/simplefolded.yaml \
	emitter-examples/simplekey1.yaml \
	emitter-examples/simplekey2.yaml \
	emitter-examples/simplekey3.yaml \
	emitter-examples/simplekey4.yaml \
	emitter-examples/simplekey5.yaml \
	emitter-examples/simplekey.yaml \
	emitter-examples/simpleliteral.yaml \
	emitter-examples/simpleseq1.yaml \
	emitter-examples/simpleseq.yaml \
	emitter-examples/simple.yaml \
	emitter-examples/singlepairimp2.yaml \
	emitter-examples/singlepairimp.yaml \
	emitter-examples/sqscalarspace.yaml \
	emitter-examples/sqscalar.yaml \
	emitter-examples/strings.yaml \
	emitter-examples/t1.yaml \
	emitter-examples/t2.yaml \
	emitter-examples/t3.yaml \
	emitter-examples/t4.yaml \
	emitter-examples/t5.yaml \
	emitter-examples/tabsmix.yaml \
	emitter-examples/tagdirective.yaml \
	emitter-examples/tagesc.yaml \
	emitter-examples/tags-1.yaml \
	emitter-examples/tags.yaml \
	emitter-examples/test1.yaml \
	emitter-examples/test2.yaml \
	emitter-examples/test.yaml \
	emitter-examples/t.yaml \
	emitter-examples/u1.yaml \
	emitter-examples/u2.yaml \
	emitter-examples/u3.yaml \
	emitter-examples/utf8-simple.yaml \
	emitter-examples/utf8.yaml \
	emitter-examples/u.yaml \
	emitter-examples/v1.yaml \
	emitter-examples/v2.yaml \
	emitter-examples/version.yaml \
	emitter-examples/v.yaml \
	emitter-examples/weirdplain.yaml \
	emitter-examples/ws0.yaml \
	emitter-examples/ws1.yaml \
	emitter-examples/ws2.yaml \
	emitter-examples/ws3.yaml \
	emitter-examples/yaml-version.yaml \
	emitter-examples/y.yaml \
	emitter-examples/yy.yaml \
	emitter-examples/zeroexplicit.yaml
