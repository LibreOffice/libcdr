#
# SYNOPSIS
#
#   DLP_FALLTHROUGH
#
# DESCRIPTION
#
#   This macro checks if the compiler supports a fallthrough warning
#   suppression attribute in GCC or CLANG style.
#
#   If a fallthrough warning suppression attribute is supported define
#   HAVE_<STYLE>_ATTRIBUTE_FALLTHROUGH.
#
#   The macro caches its result in ax_cv_have_<style>_attribute_fallthrough
#   variables.
#
# LICENSE
#
#   Copyright (c) 2017 David Tardon <dtardon@redhat.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.  This file is offered as-is, without any
#   warranty.

#serial 1

m4_defun([_DLP_FALLTHROUGH], [
    AS_VAR_PUSHDEF([ac_var], [ax_cv_have_$2_attribute_falltrough])

    AC_CACHE_CHECK([for $1], [ac_var], [
        AC_LINK_IFELSE([AC_LANG_PROGRAM([
                void foo(int &i)
                {
                    switch (i)
                    {
                        case 0:
                            i += 1;
                            $1;
                        default:
                            i += 1;
                    }
                }
            ], [])
            ],
            dnl GCC doesn't exit with an error if an unknown attribute is
            dnl provided but only outputs a warning, so accept the attribute
            dnl only if no warning were issued.
            [AS_IF([test -s conftest.err],
                [AS_VAR_SET([ac_var], [no])],
                [AS_VAR_SET([ac_var], [yes])])],
            [AS_VAR_SET([ac_var], [no])])
    ])

    AS_IF([test yes = AS_VAR_GET([ac_var])],
        [AC_DEFINE_UNQUOTED(AS_TR_CPP(HAVE_$3_ATTRIBUTE_FALLTHROUGH), 1,
            [Define to 1 if the system has the $4-style `fallthrough' attribute])], [])

    AS_VAR_POPDEF([ac_var])
])

AC_DEFUN([DLP_FALLTHROUGH], [
    _DLP_FALLTHROUGH([[__attribute__((fallthrough))]], [gcc], [GCC], [GNU])
    _DLP_FALLTHROUGH([[[[clang:fallthrough]]]], [clang], [CLANG], [clang])
])
