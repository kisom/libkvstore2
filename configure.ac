AC_PREREQ([2.60])
AC_INIT([libkvstore2],
        [0.1], 
        [coder+bugs@kyleisom.net],
        [libkvstore2],
        [https://github.com/kisom/libkvstore2])
AM_INIT_AUTOMAKE([1.11 foreign])
AC_CONFIG_SRCDIR([src/kv.h])
AC_CHECK_HEADERS
AC_CANONICAL_HOST
AC_CONFIG_FILES([Makefile src/Makefile test/Makefile])

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB
AM_PROG_CC_C_O

NO_DARWIN_MSG="
    ==============================================
    Error: darwin is not a supported platform.
    ==============================================
"
case $host_os in 
darwin*)
        AC_MSG_ERROR($NO_DARWIN_MSG)
        ;;
esac

NO_CUNIT_MSG="
    ==============================================
    Warning: CUnit was not found; will not be able
    to run unit tests!
    ==============================================
"
AC_SEARCH_LIBS([CU_initialize_registry], [cunit],
               [], [AC_MSG_WARN($NO_CUNIT_MSG)])

AC_OUTPUT
