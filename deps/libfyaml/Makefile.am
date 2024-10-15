BUILT_SOURCES = .version
.version:
	echo $(VERSION) > $@-t && mv $@-t $@
dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version
tarball-version:
	echo $(VERSION) > .tarball-version

# files to keep in the distribution (in case you want to boostrap)
EXTRA_DIST=bootstrap.sh \
	   build-aux/shave.in build-aux/shave-libtool.in \
	   build-aux/git-version-gen \
	   README.md LICENSE \
	   Dockerfile Dockerfile-build-deb \
	   Dockerfile.alpine Dockerfile.centos

MAINTAINERCLEANFILES = \
        Makefile.in src/Makefile.in config.h.in configure \
        install-sh ltmain.sh missing mkinstalldirs \
        config.log config.status config.guess config.sub config.h \
        build-stamp compile depcomp acinclude.m4 aclocal.m4 \
	stamp-h1 \
	ar-lib m4/libtool.m4 m4/ltoptions.m4 m4/ltsugar.m4 m4/ltversion.m4 \
	m4/lt~obsolete.m4 src/mock/.dirstamp src/mock/Makefile.in \
	config.h.in~ \
	test-driver test/Makefile.in \
	build-aux/ar-lib build-aux/compile build-aux/config.guess \
	build-aux/config.sub build-aux/depcomp build-aux/install-sh \
	build-aux/ltmain.sh build-aux/missing build-aux/tap-driver.sh

DISTCLEANFILES = \
	.version

clean-local:

SUBDIRS = src test doc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libfyaml.pc

maintainer-clean-local:
	@rm -rf install sphinx
	@rm -rf install artifacts

# extra file to put in the distribution
EXTRA_DIST += \
	scripts/create-virtual-env \
	scripts/install-linuxdoc.sh \
	scripts/run-check-errors.sh \
	scripts/run-compare-dump.sh \
	scripts/run-compare-examples.sh \
	scripts/run-compare-parse.sh \
	scripts/run-compare-scan.sh \
	scripts/run-compare-testsuite.sh \
	scripts/run-emit-check.sh \
	scripts/run-kcachegrind.sh \
	scripts/run-list-testsuite.sh \
	scripts/run-massif.sh \
	scripts/run-test.sh \
	scripts/run-valgrind.sh \
	scripts/show-desc.sh

if HAVE_DOCKER
docker: Dockerfile
	@DOCKER@ build -t libfyaml:$(VERSION) $(top_srcdir)
endif
