AC_PREREQ(2.61)

AC_INIT([libfyaml],
        m4_esyscmd([build-aux/git-version-gen .tarball-version]),
	[pantelis.antoniou@konsulko.com])

AC_CONFIG_SRCDIR([src/lib/fy-parse.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign 1.8.5 -Wall subdir-objects ])

m4_pattern_allow([^(AM_EXTRA_RECURSIVE_TARGETS|AM_PROG_AR)$])dnl

# supoort older versions of automake
# only define the recursive targets when it's defined
# note that the top level makefile rules will not
# include them
m4_ifdef([AM_EXTRA_RECURSIVE_TARGETS],
	 [AM_EXTRA_RECURSIVE_TARGETS([doc-help doc-html doc-latexpdf doc-man doc-clean doc-markdown])],
	 [AC_MSG_WARN([Old automake version without AM_EXTRA_RECURSIVE_TARGETS, doc-* rules won't be available at top level])])

m4_define(fyaml_major, `echo $VERSION | cut -d. -f1 | cut -d- -f1`)
m4_define(fyaml_minor, `echo $VERSION | cut -d. -f2 | cut -d- -f1`)
m4_define(fyaml_patch, `echo $VERSION | cut -d. -f3- | cut -d- -f1`)
m4_define(fyaml_extra, `echo $VERSION | cut -d- -f2- -s`)

AC_SUBST(MAJOR, fyaml_major)
AC_SUBST(MINOR, fyaml_minor)
AC_SUBST(PATCH, fyaml_patch)
AC_SUBST(EXTRA, fyaml_extra)

# libtool version is of the following format <current>:<revision>:<age>
# and it is the library ABI version
#
# - Increase current when an interface has been added removed or changed
# - Increase revision every time a release is made
# - Increase age when changes are backwards compatible
#
# examples of semantic versioning progression mapping to libtool abi versions
#
# v0.7.4 -> v0.7.5 - patch number changes, backwards compatible (1)
#                    increase revision, increase age (age must be <= current), current unchanged
#
# v0.7.5 -> v0.8.0 - minor number changes, backwards compatible (same as (1)) (2)
#                    increase revision, increase age (age must be <= current), current unchanged
#
# v0.8.0 -> v0.9.0 - minor number changes, but breaks backwards compatibility - should not happen (3)
#                    with semantic versioning for major version number >= 1, allowed for major == 0
#                    increase revision, increase current, age reset to 0
#
# v0.9.0 -> v1.0.0 - major number changes, first public release (4)
#                    increase current, set revision and age to 0
#
# v1.0.0 -> v1.0.1 - patch number changes, _must_ be backwards compatible (same as (1)) (5)
#                    increase revision, increase age (age must be <= current), current unchanged
#
# v1.0.1 -> v1.1.0 - minor number changes, backwards compatible (same as (5)) (6)
#                    increase revision, increase age (age must be <= current), current unchanged
#
# v1.1.0 -> v1.2.0 - minor number changes, breaking backwards compatibility (7)
#                    XXX illegal in semantic versioning and should not happen
#
# v1.1.0 -> v2.0.0 - major number changes, breaking backwards compatibility (same as (4)) (8)
#                    increase current, set revision and age to 0

m4_define(fyaml_libtool_version, m4_normalize(m4_include([.libtool-version])))
AC_SUBST(LIBTOOL_VERSION, fyaml_libtool_version)

dnl AX_* requires 2.64
m4_version_prereq(2.64, [AX_CHECK_ENABLE_DEBUG()], [true])

AC_PROG_MKDIR_P
AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O

AC_PROG_GCC_TRADITIONAL
AC_USE_SYSTEM_EXTENSIONS
m4_ifdef([AM_PROG_AR], [AM_PROG_AR],
	 [AC_MSG_WARN([Old automake version without AM_PROG_AR, library versioning will not be available])])

AC_PROG_CXX
AC_PROG_AWK
AC_PROG_LN_S

AX_PTHREAD

AC_PATH_PROG([M4], [m4 gm4], [no])
if test "x$M4" = xno ; then
   AC_MSG_ERROR([m4 missing])
fi

AC_SUBST(ACLOCAL_AMFLAGS, "-I m4")

# pkg-config
PKG_PROG_PKG_CONFIG

AC_LTDL_ENABLE_INSTALL
AC_LIBLTDL_INSTALLABLE
AC_LIBTOOL_DLOPEN
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_SUBST(LTDLINCL)
AC_SUBST(LIBLTDL)
AC_CONFIG_SUBDIRS(libltdl)

AC_HEADER_STDC
AC_C_CONST
AC_C_BIGENDIAN
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_TYPES(ssize_t, , [AC_DEFINE([ssize_t], [signed long],
    [Define ssize_t if it is not done by the standard libs.])])
AC_TYPE_OFF_T
AC_TYPE_SIGNAL
AC_TYPE_UID_T
AC_CHECK_DECLS(environ)

dnl for old autoconf version AX_APPEND_COMPILE_FLAGS does not work
m4_version_prereq(2.64,
	 [AX_APPEND_COMPILE_FLAGS([-Wall -Wsign-compare -Wno-stringop-overflow -fvisibility=hidden],
				  [CFLAGS], [-pedantic -Werror])],
         [CFLAGS="$CFLAGS -Wall -Wsign-compare"])

dnl -O2 is universal no need for AX_APPEND_COMPILE_FLAGS
if test "x$ax_enable_debug" != "xyes" ; then
	CFLAGS="$CFLAGS -O2"
fi

# need that as conditional because some internal tools compile as static
# to have access to internal libfyaml APIs
if test "x$enable_static" = "xyes" ; then
HAVE_STATIC=1
else
HAVE_STATIC=0
fi
AC_SUBST(HAVE_STATIC)
AC_DEFINE_UNQUOTED([HAVE_STATIC], [$HAVE_STATIC], [Define to 1 if static linking is available])
AM_CONDITIONAL([HAVE_STATIC], [ test x$HAVE_STATIC = x1 ])

dnl for old autoconf version AX_APPEND_COMPILE_FLAGS does not work
m4_version_prereq(2.64,
	 [AX_APPEND_COMPILE_FLAGS([-Wall -Wsign-compare -Wno-stringop-overflow -fvisibility=hidden],
				  [CFLAGS], [-pedantic -Werror])],
         [CFLAGS="$CFLAGS -Wall -Wsign-compare"])

dnl ASAN enable switch
AC_ARG_ENABLE([asan],
	      AS_HELP_STRING([--enable-asan],
			     [Enable ASAN support]))

HAVE_ASAN=0
ASAN_CFLAGS=""
ASAN_LIBS=""
if test "x$enable_asan" == "xyes" ; then
	AC_MSG_CHECKING([location of ASAN library])
	ASANLIB1=`${CC} -print-file-name=libasan.so`
	ASANLIB=`readlink -f "${ASANLIB1}"`
	if test -f "$ASANLIB" ; then
		HAVE_ASAN=1
		ASAN_CFLAGS="-fsanitize=address -fno-omit-frame-pointer"
		ASAN_LIBS="-fsanitize=address"
		AC_MSG_RESULT([$ASANLIB])
		m4_version_prereq(2.64,
			AX_APPEND_COMPILE_FLAGS([-fsanitize=address -fno-omit-frame-pointer], [CFLAGS]),
			[CFLAGS="$CFLAGS -fsanitize=address -fno-omit-frame-pointer"])
	else
		AC_MSG_RESULT([Not found; disabling ASAN])
	fi
fi
AC_SUBST(HAVE_ASAN)
AC_SUBST(ASAN_CFLAGS)
AC_SUBST(ASAN_LIBS)
AM_CONDITIONAL([HAVE_ASAN],
	       [ test x$HAVE_ASAN = x1 ])
AC_DEFINE_UNQUOTED([HAVE_ASAN], [$HAVE_ASAN], [Define to 1 if ASAN is enabled])

# include -lm in the link
AC_SEARCH_LIBS([llrintf], [m], [], [AC_MSG_ERROR([unable to find the llrintf() function])])

# check if there's a qsort_r available (musl does not have it)
AC_CHECK_FUNC([qsort_r],
	      HAVE_QSORT_R=1,
	      HAVE_QSORT_R=0)
AC_SUBST(HAVE_QSORT_R)
AC_DEFINE_UNQUOTED([HAVE_QSORT_R], [$HAVE_QSORT_R], [Define to 1 if you have qsort_r available])
AM_CONDITIONAL([HAVE_QSORT_R], [ test x$HAVE_QSORT_R = x1 ])

PKG_CHECK_MODULES(LIBYAML, [ yaml-0.1 ], HAVE_LIBYAML=1, HAVE_LIBYAML=0)

# update with pkg-config's flags
if test "x$HAVE_LIBYAML" != "x1" ; then
	AC_MSG_WARN([failed to find libyaml; compatibility disabled])
fi

AC_SUBST(HAVE_LIBYAML)
AC_SUBST(LIBYAML_CFLAGS)
AC_SUBST(LIBYAML_LIBS)
AC_DEFINE_UNQUOTED([HAVE_LIBYAML], [$HAVE_LIBYAML], [Define to 1 if you have libyaml available])
AM_CONDITIONAL([HAVE_LIBYAML], [ test x$HAVE_LIBYAML = x1 ])

PKG_CHECK_MODULES(CHECK, [ check ], HAVE_CHECK=1, HAVE_CHECK=0)

AC_SUBST(HAVE_CHECK)
AC_SUBST(CHECK_CFLAGS)
AC_SUBST(CHECK_LDFLAGS)
AC_SUBST(CHECK_LIBS)
AC_DEFINE_UNQUOTED([HAVE_CHECK], [$HAVE_CHECK], [Define to 1 if you have check available])
AM_CONDITIONAL([HAVE_CHECK], [ test x$HAVE_CHECK = x1 ])

HAVE_COMPATIBLE_CHECK=0
if test "x$HAVE_CHECK" = "x1" ; then
	save_LIBS="$LIBS"
	save_CFLAGS="$CFLAGS"
	LIBS="$LIBS $CHECK_LIBS"
	CFLAGS="$CFLAGS $CHECK_CFLAGS"

	# check if libcheck has srunner_set_tap (jessie has outdated libcheck)
	AC_CHECK_FUNC([srunner_set_tap],
		      HAVE_COMPATIBLE_CHECK=1,
		      HAVE_COMPATIBLE_CHECK=0)

	LIBS="$save_LIBS"
	CFLAGS="$save_CFLAGS"
fi

AC_SUBST(HAVE_COMPATIBLE_CHECK)
AC_DEFINE_UNQUOTED([HAVE_COMPATIBLE_CHECK], [$HAVE_COMPATIBLE_CHECK], [Define to 1 if you have a compatible version of check available])
AM_CONDITIONAL([HAVE_COMPATIBLE_CHECK], [ test x$HAVE_COMPATIBLE_CHECK = x1 ])

dnl enable internet available when checking
AC_ARG_ENABLE([network],
	      AS_HELP_STRING([--disable-network],
			     [Disable tests requiring network access]))

if test "x$enable_network" != "xno" ; then
	HAVE_NETWORK=1
else
	HAVE_NETWORK=0
fi
AC_SUBST(HAVE_NETWORK)
AM_CONDITIONAL([HAVE_NETWORK],
	       [ test x$HAVE_NETWORK = x1 ])

dnl enable internet available when checking
AC_ARG_ENABLE([devmode],
	      AS_HELP_STRING([--enable-devmode],
			     [Enable development mode only debugging]))

if test "x$enable_devmode" != "xno" ; then
	HAVE_DEVMODE=1
else
	HAVE_DEVMODE=0
fi
AC_SUBST(HAVE_DEVMODE)
AM_CONDITIONAL([HAVE_DEVMODE],
	       [ test x$HAVE_DEVMODE = x1 ])
AC_DEFINE_UNQUOTED([HAVE_DEVMODE], [$HAVE_DEVMODE], [Define to 1 if developer mode is enabled])

# check for sphinx
AC_PATH_PROG([SPHINX], [sphinx-build])

HAVE_SPHINX=0
if test "x$SPHINX" != "x" ; then
	AC_PATH_PROG([PIP3], [pip3])

	# if both pip3 and sphinx-build are available check for versions
	if test "x$PIP3" != "x" ; then
		AC_MSG_CHECKING([sphinx version])
		SPHINX_VERSION=`pip3 2>/dev/null show sphinx | grep -i Version: | cut -d: -f2 | sed -e 's/ //g'`
		if test "x$SPHINX_VERSION" != "x" ; then
			AC_MSG_RESULT([$SPHINX_VERSION])
		else
			AC_MSG_RESULT([N/A])
		fi

		AC_MSG_CHECKING([sphinx RTD theme version])
		SPHINX_RTD_THEME_VERSION=`pip3 2>/dev/null show sphinx_rtd_theme | grep -i Version: | cut -d: -f2 | sed -e 's/ //g'`
		if test "x$SPHINX_VERSION" != "x" ; then
			AC_MSG_RESULT([$SPHINX_RTD_THEME_VERSION])
		else
			AC_MSG_RESULT([N/A])
		fi

		AC_MSG_CHECKING([sphinx markdown builder version])
		SPHINX_MARKDOWN_BUILDER_VERSION=`pip3 2>/dev/null show sphinx-markdown-builder | grep -i Version: | cut -d: -f2 | sed -e 's/ //g'`
		if test "x$SPHINX_MARKDOWN_BUILDER_VERSION" != "x" ; then
			AC_MSG_RESULT([$SPHINX_MARKDOWN_BUILDER_VERSION])
		else
			AC_MSG_RESULT([N/A])
		fi

		AC_MSG_CHECKING([sphinx linuxdoc version])
		SPHINX_LINUXDOC_VERSION=`pip3 2>/dev/null show linuxdoc | grep -i Version: | cut -d: -f2 | sed -e 's/ //g'`
		if test "x$SPHINX_LINUXDOC_VERSION" != "x" ; then
			AC_MSG_RESULT([$SPHINX_LINUXDOC_VERSION])
		else
			AC_MSG_RESULT([N/A])
		fi

		if test "x$SPHINX_VERSION" != "x" -a "x$SPHINX_RTD_THEME_VERSION" != "x" -a "x$SPHINX_MARKDOWN_BUILDER_VERSION" != "x" -a "x$SPHINX_LINUXDOC_VERSION" != "x" ; then
			HAVE_SPHINX=1
		fi
	fi

	AC_MSG_CHECKING([whether sphinx installation works])
	if test "x$HAVE_SPHINX" = "x1" ; then
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
	fi
fi

AC_DEFINE_UNQUOTED([HAVE_SPHINX], [$HAVE_SPHINX], [Define to 1 if you have sphinx (and all required packages) available])
AM_CONDITIONAL([HAVE_SPHINX], [ test x$HAVE_SPHINX = x1 ])

# check for git
AC_PATH_PROG([GIT], [git])
if test "x$GIT" != "x" ; then
	HAVE_GIT=1
else
	HAVE_GIT=0
fi
AC_DEFINE_UNQUOTED([HAVE_GIT], [$HAVE_GIT], [Define to 1 if you have git available])
AM_CONDITIONAL([HAVE_GIT], [ test x$HAVE_GIT = x1 ])

AC_ARG_VAR(TESTSUITEURL, [Testsuite git repo URL (default: https://github.com/yaml/yaml-test-suite)])

if test "x$TESTSUITEURL" = "x" ; then
	TESTSUITEURL="https://github.com/yaml/yaml-test-suite"
fi

AC_ARG_VAR(TESTSUITECHECKOUT, [Testsuite checkout (default: 6e6c296ae9c9d2d5c4134b4b64d01b29ac19ff6f)])

if test "x$TESTSUITECHECKOUT" = "x" ; then
	TESTSUITECHECKOUT="6e6c296ae9c9d2d5c4134b4b64d01b29ac19ff6f"
fi

AC_ARG_VAR(JSONTESTSUITEURL, [JSON Testsuite git repo URL (default: https://github.com/nst/JSONTestSuite)])

if test "x$JSONTESTSUITEURL" = "x" ; then
	JSONTESTSUITEURL="https://github.com/nst/JSONTestSuite"
fi

AC_ARG_VAR(JSONTESTSUITECHECKOUT, [JSON Testsuite checkout (default: d64aefb55228d9584d3e5b2433f720ea8fd00c82)])

if test "x$JSONTESTSUITECHECKOUT" = "x" ; then
	JSONTESTSUITECHECKOUT="d64aefb55228d9584d3e5b2433f720ea8fd00c82"
fi

# check for docker
AC_PATH_PROG([DOCKER], [docker])
if test "x$DOCKER"  != "x" ; then
	HAVE_DOCKER=1
else
	HAVE_DOCKER=0
fi
AM_CONDITIONAL([HAVE_DOCKER], [ test x$HAVE_DOCKER = x1 ])

# check for jq
AC_PATH_PROG([JQ], [jq])
if test "x$JQ" != "x" ; then
	HAVE_JQ=1
else
	HAVE_JQ=0
fi
AC_DEFINE_UNQUOTED([HAVE_JQ], [$HAVE_JQ], [Define to 1 if you have jq available])
AM_CONDITIONAL([HAVE_JQ], [ test x$HAVE_JQ = x1 ])
AC_SUBST(JQ, "$JQ")

# Shave by default on supported autoconf versions
dnl m4_version_prereq(2.64, SHAVE_INIT([build-aux], [enable]), [true])
m4_ifdef([SHAVE_INIT], [SHAVE_INIT([build-aux], [enable])])

AC_CONFIG_FILES([
        build-aux/shave
        build-aux/shave-libtool
	Makefile
	src/Makefile
	test/Makefile
	doc/Makefile
	libfyaml.pc
])

AC_REQUIRE_AUX_FILE([tap-driver.sh])

AC_OUTPUT

echo "
 ---{ $PACKAGE_NAME $VERSION }---

    VERSION:               ${VERSION}
    MAJOR.MINOR:           ${MAJOR}.${MINOR}
    PATCH:                 ${PATCH}
    EXTRA:                 ${EXTRA}
    LIBTOOL_VERSION:       ${LIBTOOL_VERSION}
    prefix:                ${prefix}
    C compiler:            ${CC}
    CFLAGS:                ${CFLAGS}
    Linker:                ${LD}
    LDFLAGS:               ${LDFLAGS}
    LIBS:                  ${LIBS}
    HAVE_ASAN:             ${HAVE_ASAN}
    HAVE_CHECK:            ${HAVE_CHECK}
    HAVE_COMPATIBLE_CHECK: ${HAVE_COMPATIBLE_CHECK}
    HAVE_NETWORK:          ${HAVE_NETWORK}
    HAVE_DEVMODE:          ${HAVE_DEVMODE}
    HAVE_SPHINX:           ${HAVE_SPHINX}
    GIT:                   $GIT
    DOCKER:                $DOCKER
    TESTSUITEURL:          $TESTSUITEURL
    TESTSUITECHECKOUT:     $TESTSUITECHECKOUT
    JSONTESTSUITEURL:      $JSONTESTSUITEURL
    JSONTESTSUITECHECKOUT: $JSONTESTSUITECHECKOUT

"
