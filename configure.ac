#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT(mxflib, alpha-0.3.3)
AC_CONFIG_SRCDIR([mxflib/mxffile.cpp])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE(mxflib, alpha-0.3.3)

# Checks for programs.
AC_PROG_CXX
AC_PROG_RANLIB

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lmxf':
AC_CHECK_LIB([mxf], [LoadTypes])
UUIDLIB=""
AC_CHECK_HEADER([uuid/uuid.h],
			[AC_CHECK_LIB(uuid, uuid_generate, 
				[AC_DEFINE([HAVE_UUID_GENERATE], 1, [ Have uuid_generate])
				have_libuuid=yes UUIDLIB="-luuid" ], [have_libuuid=no])])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/time.h sys/timeb.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FSEEKO
AC_FUNC_MEMCMP
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([floor gettimeofday memmove memset strcasecmp strerror strtoul])

DEFAULT_INCLUDES=""
CXXFLAGS="-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -DDATADIR=\\\"\$(datadir)/mxflib\\\" -g -Wall"
AC_SUBST(UUIDLIB)
AC_SUBST(DEFAULT_INCLUDES)
AC_SUBST(CXXFLAGS)

AC_CONFIG_TESTDIR([tests], [tests:mxfdump:mxfwrap:mxfsplit])
AC_CONFIG_FILES([tests/Makefile tests/atlocal])

AC_CONFIG_FILES([Makefile
                 docs/Makefile
                 mxfdump/Makefile
                 mxflib/Makefile
                 mxfsplit/Makefile
                 mxfwrap/Makefile])
AM_MISSING_PROG([AUTOM4TE], [autom4te])

AC_OUTPUT