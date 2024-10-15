# ===========================================================================
#       http://www.gnu.org/software/autoconf-archive/ax_check_flag.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_CHECK_PREPROC_FLAG(FLAG, [ACTION-SUCCESS], [ACTION-FAILURE], [EXTRA-FLAGS])
#   AX_CHECK_COMPILE_FLAG(FLAG, [ACTION-SUCCESS], [ACTION-FAILURE], [EXTRA-FLAGS])
#   AX_CHECK_LINK_FLAG(FLAG, [ACTION-SUCCESS], [ACTION-FAILURE], [EXTRA-FLAGS])
#   AX_APPEND_FLAG(FLAG, [FLAGS-VARIABLE])
#   AX_APPEND_COMPILE_FLAGS([FLAG1 FLAG2 ...], [FLAGS-VARIABLE], [EXTRA-FLAGS])
#   AX_APPEND_LINK_FLAGS([FLAG1 FLAG2 ...], [FLAGS-VARIABLE], [EXTRA-FLAGS])
#
# DESCRIPTION
#
#   Check whether the given FLAG works with the current language's
#   preprocessor/compiler/linker, or whether they give an error. (Warnings,
#   however, are ignored.)
#
#   ACTION-SUCCESS/ACTION-FAILURE are shell commands to execute on
#   success/failure.
#
#   If EXTRA-FLAGS is defined, it is added to the current language's default
#   flags (e.g.  CFLAGS) when the check is done.  The check us thus made
#   with the following flags: "CFLAGS EXTRA-FLAGS FLAG".  EXTRA-FLAGS can
#   for example be used to force the compiler to issue an error when a bad
#   flag is given.
#
#   AX_APPEND_FLAG appends the FLAG to the FLAG-VARIABLE shell variable or
#   the current language's flags if not specified.  FLAG is not added to
#   FLAG-VARIABLE if it is already in the shell variable.
#
#   AX_APPEND_COMPILE_FLAGS checks for each FLAG1, FLAG2, etc. using
#   AX_CHECK_COMPILE_FLAG and if the check is successful the flag is added
#   to the appropriate FLAGS variable with AX_APPEND_FLAG.  The
#   FLAGS-VARIABLE and EXTRA-FLAGS arguments are the same as in the other
#   macros. AX_APPEND_LINK_FLAGS does the same for linker flags.
#
#   NOTE: Based on AX_CHECK_COMPILER_FLAGS and AX_CFLAGS_GCC_OPTION.
#
# LICENSE
#
#   Copyright (c) 2008 Guido U. Draheim <guidod@gmx.de>
#   Copyright (c) 2009 Steven G. Johnson <stevenj@alum.mit.edu>
#   Copyright (c) 2009 Matteo Frigo
#   Copyright (c) 2011 Maarten Bosmans <mkbosmans@gmail.com>
#
#   This program is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation, either version 3 of the License, or (at your
#   option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#   Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program. If not, see <http://www.gnu.org/licenses/>.
#
#   As a special exception, the respective Autoconf Macro's copyright owner
#   gives unlimited permission to copy, distribute and modify the configure
#   scripts that are the output of Autoconf when processing the Macro. You
#   need not follow the terms of the GNU General Public License when using
#   or distributing such scripts, even though portions of the text of the
#   Macro appear in them. The GNU General Public License (GPL) does govern
#   all other use of the material that constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the Autoconf
#   Macro released by the Autoconf Archive. When you make and distribute a
#   modified version of the Autoconf Macro, you may extend this special
#   exception to the GPL to apply to your modified version as well.

#serial 2

AC_DEFUN([AX_CHECK_PREPROC_FLAG],
[AC_PREREQ(2.59) dnl for _AC_LANG_PREFIX
AS_VAR_PUSHDEF([CACHEVAR],[ax_cv_check_[]_AC_LANG_ABBREV[]cppflags_$4_$1])dnl
AC_CACHE_CHECK([whether _AC_LANG preprocessor accepts $1], CACHEVAR, [
  ax_check_save_flags=$CPPFLAGS
  CPPFLAGS="$CPPFLAGS $4 $1"
  AC_PREPROC_IFELSE([AC_LANG_PROGRAM()],
    [AS_VAR_SET([CACHEVAR],[yes])],
    [AS_VAR_SET([CACHEVAR],[no])])
  CPPFLAGS=$ax_check_save_flags])
AS_VAR_IF([CACHEVAR], "yes",
  [m4_default([$2], :)],
  [m4_default([$3], :)])
AS_VAR_POPDEF([CACHEVAR])dnl
])dnl AX_CHECK_PREPROC_FLAGS

AC_DEFUN([AX_CHECK_COMPILE_FLAG],
[AC_PREREQ(2.59) dnl for _AC_LANG_PREFIX
AS_VAR_PUSHDEF([CACHEVAR],[ax_cv_check_[]_AC_LANG_ABBREV[]flags_$4_$1])dnl
AC_CACHE_CHECK([whether _AC_LANG compiler accepts $1], CACHEVAR, [
  ax_check_save_flags=$[]_AC_LANG_PREFIX[]FLAGS
  _AC_LANG_PREFIX[]FLAGS="$[]_AC_LANG_PREFIX[]FLAGS $4 $1"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM()],
    [AS_VAR_SET([CACHEVAR],[yes])],
    [AS_VAR_SET([CACHEVAR],[no])])
  _AC_LANG_PREFIX[]FLAGS=$ax_check_save_flags])
AS_VAR_IF([CACHEVAR], "yes",
  [m4_default([$2], :)],
  [m4_default([$3], :)])
AS_VAR_POPDEF([CACHEVAR])dnl
])dnl AX_CHECK_COMPILE_FLAGS

AC_DEFUN([AX_CHECK_LINK_FLAG],
[AS_VAR_PUSHDEF([CACHEVAR],[ax_cv_check_ldflags_$4_$1])dnl
AC_CACHE_CHECK([whether the linker accepts $1], CACHEVAR, [
  ax_check_save_flags=$LDFLAGS
  LDFLAGS="$LDFLAGS $4 $1"
  AC_LINK_IFELSE([AC_LANG_PROGRAM()],
    [AS_VAR_SET([CACHEVAR],[yes])],
    [AS_VAR_SET([CACHEVAR],[no])])
  LDFLAGS=$ax_check_save_flags])
AS_VAR_IF([CACHEVAR], "yes",
  [m4_default([$2], :)],
  [m4_default([$3], :)])
AS_VAR_POPDEF([CACHEVAR])dnl
])dnl AX_CHECK_LINK_FLAGS


AC_DEFUN([AX_APPEND_FLAG],
[AC_PREREQ(2.59) dnl for _AC_LANG_PREFIX
AC_REQUIRE([AC_PROG_GREP])
AS_VAR_PUSHDEF([FLAGS], [m4_default($2,_AC_LANG_PREFIX[]FLAGS)])dnl
AS_VAR_SET_IF([FLAGS],
  [AS_IF([AS_ECHO(" $[]FLAGS ") | $GREP " $1 " 2>&1 >/dev/null],
    [AC_RUN_LOG([: FLAGS already contains $1])],
    [AC_RUN_LOG([: FLAGS="$FLAGS $1"])
    AS_VAR_APPEND([FLAGS], [" $1"])])],
  [AS_VAR_SET([FLAGS],[$1])])
AS_VAR_POPDEF([FLAGS])dnl
])dnl AX_APPEND_FLAG

AC_DEFUN([AX_APPEND_COMPILE_FLAGS],
[for flag in $1; do
  AX_CHECK_COMPILE_FLAG([$flag], [AX_APPEND_FLAG([$flag], [$2])], [], [$3])
done
])dnl AX_APPEND_COMPILE_FLAGS

AC_DEFUN([AX_APPEND_LINK_FLAGS],
[for flag in $1; do
  AX_CHECK_LINK_FLAG([$flag], [AX_APPEND_FLAG([$flag], [m4_default([$2], [LDFLAGS])])], [], [$3])
done
])dnl AX_APPEND_LINK_FLAGS
