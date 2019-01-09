# --------------------------------------------------------------------------- #
# Econet FileStore server and gateway                                         #
#                                                                             #
# (c) Eelco Huininga 2017-2019                                                #
# --------------------------------------------------------------------------- #

# --------------------------------------------------------------------------- #
# Generic functions                                                           #
# --------------------------------------------------------------------------- #
# ===========================================================================
#  https://www.gnu.org/software/autoconf-archive/ax_check_compile_flag.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_CHECK_COMPILE_FLAG(FLAG, [ACTION-SUCCESS], [ACTION-FAILURE], [EXTRA-FLAGS], [INPUT])
#
# DESCRIPTION
#
#   Check whether the given FLAG works with the current language's compiler
#   or gives an error.  (Warnings, however, are ignored)
#
#   ACTION-SUCCESS/ACTION-FAILURE are shell commands to execute on
#   success/failure.
#
#   If EXTRA-FLAGS is defined, it is added to the current language's default
#   flags (e.g. CFLAGS) when the check is done.  The check is thus made with
#   the flags: "CFLAGS EXTRA-FLAGS FLAG".  This can for example be used to
#   force the compiler to issue an error when a bad flag is given.
#
#   INPUT gives an alternative input source to AC_COMPILE_IFELSE.
#
#   NOTE: Implementation based on AX_CFLAGS_GCC_OPTION. Please keep this
#   macro in sync with AX_CHECK_{PREPROC,LINK}_FLAG.
#
# LICENSE
#
#   Copyright (c) 2008 Guido U. Draheim <guidod@gmx.de>
#   Copyright (c) 2011 Maarten Bosmans <mkbosmans@gmail.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.  This file is offered as-is, without any
#   warranty.

#serial 6

AC_DEFUN([AX_CHECK_COMPILE_FLAG],
[AC_PREREQ(2.64)dnl for _AC_LANG_PREFIX and AS_VAR_IF
AS_VAR_PUSHDEF([CACHEVAR],[ax_cv_check_[]_AC_LANG_ABBREV[]flags_$4_$1])dnl
AC_CACHE_CHECK([whether _AC_LANG compiler accepts $1], CACHEVAR, [
  ax_check_save_flags=$[]_AC_LANG_PREFIX[]FLAGS
  _AC_LANG_PREFIX[]FLAGS="$[]_AC_LANG_PREFIX[]FLAGS $4 $1"
  AC_COMPILE_IFELSE([m4_default([$5],[AC_LANG_PROGRAM()])],
    [AS_VAR_SET(CACHEVAR,[yes])],
    [AS_VAR_SET(CACHEVAR,[no])])
  _AC_LANG_PREFIX[]FLAGS=$ax_check_save_flags])
AS_VAR_IF(CACHEVAR,yes,
  [m4_default([$2], :)],
  [m4_default([$3], :)])
AS_VAR_POPDEF([CACHEVAR])dnl
])dnl AX_CHECK_COMPILE_FLAGS

# --------------------------------------------------------------------------- #
# FileStore specific data                                                     #
# --------------------------------------------------------------------------- #

# --------------------------------------------------------------------------- #
# Initialize autoconf                                                         #
# --------------------------------------------------------------------------- #
AC_PREREQ([2.69])
AC_INIT(FileStore, m4_esyscmd([cat VERSION | tr -d '\n']))

# --------------------------------------------------------------------------- #
# 
# --------------------------------------------------------------------------- #
FILESTORE_ADAPTER_NONE=0
FILESTORE_ADAPTER_REMA=1
AC_SUBST(MAIN_SRCS, "\\
	main.cpp \\
	adfs.cpp \\
	aun.cpp \\
	cli.cpp \\
	debug.cpp \\
	econet.cpp \\
	errorhandler.cpp \\
	fs_native.cpp \\
	netfs.cpp \\
	settings.cpp \\
	stations.cpp \\
	users.cpp \\
	platforms/linux.cpp")
AC_SUBST(MAIN_EXECUTABLE, "FileStore")
OPENSSL_KEYSIZE="4096"
OPENSSL_ECDHCURVE="x25519"
OPENSSL_HASH="SHA512"
OPENSSL_EXPIRE="3650"
OPENSSL_CA_DIR="../conf/keys/"
OPENSSL_SUBJ="/C=UK/ST=Cambridgeshire/L=Cambridge/O=Acorn"
OPENSSL_CLIENT_KEY="client.key"
OPENSSL_CLIENT_CERT="client.cert"
OPENSSL_SERVER_KEY="server.key"
OPENSSL_SERVER_CERT="server.cert"

# --------------------------------------------------------------------------- #
# Define custom command line options                                          #
# --------------------------------------------------------------------------- #
AC_ARG_WITH([adapter],
	[AS_HELP_STRING(
		[--with-adapter],
		[Enable hardware support for external Econet adapters (e.g. the RaspberryPi Econet Module Adapter).]
	)], [
	], [
		FILESTORE_ADAPTER=""
	]
)
AC_ARG_ENABLE([debug],
	[AS_HELP_STRING(
		[--enable-debug],
		[Enable debug mode [default=no]]
	)], [
		FILESTORE_DEBUG=1
	], [
		FILESTORE_DEBUG=0
	]
)
AC_ARG_ENABLE([ipv6],
	[AS_HELP_STRING(
		[--enable-ipv6],
		[Enable IPv6 networking [default=no]]
	)], [
		FILESTORE_WITHIPV6=1
	], [
		FILESTORE_WITHIPV6=0
	]
)

# --------------------------------------------------------------------------- #
# Get version number and build date/time                                      #
# --------------------------------------------------------------------------- #
FILESTORE_VERSION=`cat $srcdir/VERSION`
FILESTORE_VERSION_MAJOR=`echo $FILESTORE_VERSION | cut -d. -f1`
FILESTORE_VERSION_MINOR=`echo $FILESTORE_VERSION | cut -d. -f2`
FILESTORE_VERSION_PATCHLEVEL=`echo $FILESTORE_VERSION | cut -d. -f3`
FILESTORE_VERSION_BUILDDATE="`date -u '+%Y-%m-%d %H:%M:%S'` UTC"
AC_MSG_RESULT([-------------------------------------------------------------------------------])
AC_MSG_RESULT([ FileStore version $FILESTORE_VERSION                  build date $FILESTORE_VERSION_BUILDDATE])
AC_MSG_RESULT([-------------------------------------------------------------------------------])

# --------------------------------------------------------------------------- #
# Location of main file                                                       #
# --------------------------------------------------------------------------- #
AC_CONFIG_SRCDIR([main.cpp])

# --------------------------------------------------------------------------- #
# Checks for compiler programs
# --------------------------------------------------------------------------- #
#AC_PROG_CC
AC_PROG_CXX
AC_PROG_MAKE_SET

# --------------------------------------------------------------------------- #
# Check if we can set some additional compiler settings                       #
# --------------------------------------------------------------------------- #
AX_CHECK_COMPILE_FLAG([-std=c++11],	[CXXFLAGS="$CXXFLAGS -std=c++11"])
AX_CHECK_COMPILE_FLAG([-Wall],		[CXXFLAGS="$CXXFLAGS -Wall"])
AX_CHECK_COMPILE_FLAG([-Wextra],	[CXXFLAGS="$CXXFLAGS -Wextra"])
AX_CHECK_COMPILE_FLAG([-Wpedantic],	[CXXFLAGS="$CXXFLAGS -Wpedantic"])

# --------------------------------------------------------------------------- #
# Enable compiler optimizations
# --------------------------------------------------------------------------- #
#AX_CHECK_COMPILE_FLAG([-O2], [CFLAGS="$CFLAGS -O2"])

# --------------------------------------------------------------------------- #
# Checks for typedefs, structures, and compiler characteristics.
# --------------------------------------------------------------------------- #
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UID_T
AC_TYPE_UINT8_T

# --------------------------------------------------------------------------- #
# Checks for header files.
# --------------------------------------------------------------------------- #
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netinet/in.h stdint.h stdlib.h string.h sys/socket.h unistd.h])
# Check for SSL headers
AC_CHECK_HEADERS([openssl/ssl.h])

# --------------------------------------------------------------------------- #
# Checks for library functions.
# --------------------------------------------------------------------------- #
AC_FUNC_CHOWN
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([bzero inet_ntoa memmove memset select socket strcasecmp strerror strtol strtoul strlcpy strtoupper kbhit readline])
AS_IF(	[test "x$ac_cv_func_strlcpy" = "xno"],
	[AC_SUBST(MAIN_SRCS, "${MAIN_SRCS} \\
	platforms/strlcpy.cpp")
	AC_SUBST(FILESTORE_HAS_STRLCPY, 0)],
	[AC_SUBST(FILESTORE_HAS_STRLCPY, 1)]
)
AS_IF(	[test "x$ac_cv_func_strtoupper" = "xno"],
	[AC_SUBST(MAIN_SRCS, "${MAIN_SRCS} \\
	platforms/strtoupper.cpp")
	AC_SUBST(FILESTORE_HAS_STRTOUPPER, 0)],
	[AC_SUBST(FILESTORE_HAS_STRTOUPPER, 1)]
)
AS_IF(	[test "x$ac_cv_func_kbhit" = "xno"],
	[AC_SUBST(MAIN_SRCS, "${MAIN_SRCS} \\
	platforms/kbhit.cpp")
	AC_SUBST(FILESTORE_HAS_KBHIT, 0)],
	[AC_SUBST(FILESTORE_HAS_KBHIT, 1)]
)

# Checks for pthread library
AC_CHECK_LIB(
	[pthread],
	[pthread_mutex_init],
	[LIBS="$LIBS -lpthread"],
	[AC_MSG_ERROR([Cannot find -lpthread])]
)
# Checks for readline library
AC_CHECK_LIB(
	[readline],
	[readline],
	[LIBS="$LIBS -lreadline"],
	[AC_MSG_ERROR([Cannot find -lreadline])]
)
# Checks for SSL libraries
AC_CHECK_LIB([crypto],
	[main],
	[LIBS="$LIBS -lcrypto"],
	[]
)
AC_CHECK_LIB([ssl],
	[SSL_get_version],
	[LIBS="$LIBS -lssl"],
	[]
)

# --------------------------------------------------------------------------- #
# Check if building with debug support is requested                           #
# --------------------------------------------------------------------------- #
AC_MSG_CHECKING([if additional debug information is requested])
AC_ARG_ENABLE(
	[debug],
	[AS_HELP_STRING(
		[--enable-debug],
		[Enable compiling with debug information [default=no]]
	)],
	[	AC_MSG_RESULT([yes])
		AX_CHECK_COMPILE_FLAG(
			[-g],
			[CXXFLAGS="$CXXFLAGS -g"]
		)
		FILESTORE_DEBUG=1
	],
	[	AC_MSG_RESULT([no])
		FILESTORE_DEBUG=0
	]
)

# --------------------------------------------------------------------------- #
# Check if support for secure networks (OpenSSL) is requested                 #
# --------------------------------------------------------------------------- #
AC_MSG_CHECKING([if OpenSSL support is requested])
AC_ARG_ENABLE(
	[tls],
	[AS_HELP_STRING(
		[--enable-tls],
		[Enable TLS support (secure AUN networks) [default=no]]
	)],
	[	AC_MSG_RESULT([yes])
		FILESTORE_WITHOPENSSL=1
		AC_SUBST(MAIN_SRCS, "${MAIN_SRCS} \\
		dtls/dtls.cpp")
	],
	[	AC_MSG_RESULT([no])
		FILESTORE_WITHOPENSSL=0
	]

)

# --------------------------------------------------------------------------- #
# Check if a custom location for SSL headers and libraries are requested      #
# --------------------------------------------------------------------------- #
AC_ARG_WITH(
	[sslinc],
	[AS_HELP_STRING(
		[--with-sslinc=PATH],
		[Path to OpenSSL headers; requires --enable-tls]
	)], [
		AS_IF([test "$enable_tls" != "no"],
		[	AS_IF([test -d "$withval"],
			[	save_CC="$CC"
				save_CPP="$CPP"
				CC="$CC -I$withval"
				CPP="$CPP -I$withval"
				AC_CHECK_HEADERS(
					[openssl/ssl.h openssl/x509v3.h],
					[	AC_SUBST(INCLUDES, -I$withval)],
					[	AC_MSG_WARN([Invalid path to OpenSSL headers. $withval/openssl/ doesn't contain the required files.])
						sslinc=""
						break
				        ],
					[]
				)
				CC="$save_CC"
				CPP="$save_CPP"
			], [
				AC_MSG_WARN([Invalid path to OpenSSL headers, $withval is not a directory.])
			])
		], [
		])
	]
)

AC_ARG_WITH([ssllib],
	[AS_HELP_STRING(
		[--with-ssl-lib=PATH],
		[Path to OpenSSL libraries; requires --enable-tls]
	)], [
		AS_IF([test "$enable_tls" != "no"],
		[	AS_IF([test -d "$withval"],
			[	AC_CHECK_LIB(
					[crypto],
					[X509_digest],
					[],
					[havessllib="no"],
					[-L$withval -lssl]
				)
				AC_CHECK_LIB(
					[ssl],
					[SSL_accept],
					[],
					[havessllib="no"],
					[-L$withval -lcrypto]
				)
				AS_IF([test "x$havessllib" = "xno"],
					[AC_MSG_WARN([Invalid path to OpenSSL libs. $withval doesn't contain the required files.])],
					[AC_SUBST(LDFLAGS, [${LDFLAGS} -L$withval])]
				)
			], [	AC_MSG_WARN([Invalid path to OpenSSL libraries, $withval is not a directory.])]
			)
		],
		[])
	]
)

# --------------------------------------------------------------------------- #
# Check if we need support for the RaspberryPi Econet Module Adapter (PiGPIO) #
# --------------------------------------------------------------------------- #
AC_MSG_CHECKING([if support for hardware adapters is requested])
AS_IF([test "x$with_adapter" != "x"], [
	AS_CASE([x$with_adapter],
		[xrema],
		[	AC_MSG_RESULT([RaspberryPi Econet Module Adapter])
			AS_IF([test "$host" = "" || test "$host" = "arm-linux-gnueabi"],
				[	AC_SUBST(FILESTORE_ADAPTER, 1)
					AC_SUBST(MAIN_SRCS, "${MAIN_SRCS} \\
	platforms/adapters/rpi-gpio.cpp")

					# Check if we can find the PiGPIO header
					save_CC="$CC"
					save_CPP="$CPP"
					CC="$CC -I."
					CPP="$CPP -I."
					AC_CHECK_HEADERS(
						[pigpio/pigpio.h],
						[AC_SUBST(INCLUDES, "${INCLUDES} -I./pigpio/")],
						[AC_MSG_ERROR([Cannot find pigpio.h. Please download the PiGPIO library first (git clone https://github.com/joan2937/pigpio/).])],
						[]
					)
					CC="$save_CC"
					CPP="$save_CPP"

					# Checks we can find the PiGPIO libraries
					AC_CHECK_LIB(
						[pigpio],
						[gpioInitialise],
						[	AC_SUBST(LIBS, "${LIBS} -lpigpio")
							AC_SUBST(LDFLAGS, "${LDFLAGS} -L./pigpio/")],
						[AC_MSG_ERROR([Cannot find -lpigpio (libpigpio.so). Please compile the PiGPIO library in $(pwd)/pigpio/ first.])],
						[-L./pigpio/ -lpigpio]
					) ],
				[AC_MSG_ERROR([Can't build RaspberryPi binaries for $host])]
			) ],

		[xnone],
		[	AC_MSG_RESULT([no])
			AC_SUBST(FILESTORE_ADAPTER, 0)
			AC_SUBST(MAIN_SRCS, "${MAIN_SRCS} \\
	platforms/adapters/none.cpp")],

		[	AC_MSG_RESULT([unknown adapter])
			AC_MSG_ERROR([Unknown value x$with_adapter for --with-adapter])]
	)],
	[	AC_MSG_RESULT([no])
		AC_SUBST(FILESTORE_ADAPTER, 0)
		AC_SUBST(MAIN_SRCS, "${MAIN_SRCS} \\
	platforms/adapters/none.cpp")]
)

# --------------------------------------------------------------------------- #
# Create the files
# --------------------------------------------------------------------------- #
AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(INCLUDES)
AC_SUBST(LIBS)
AC_SUBST(host)
AC_SUBST(FILESTORE_VERSION_MAJOR)
AC_SUBST(FILESTORE_VERSION_MINOR)
AC_SUBST(FILESTORE_VERSION_PATCHLEVEL)
AC_SUBST(FILESTORE_VERSION_BUILDDATE)
AC_SUBST(FILESTORE_HAS_KBHIT)
AC_SUBST(FILESTORE_HAS_STRLCPY)
AC_SUBST(FILESTORE_HAS_STRTOUPPER)
AC_SUBST(FILESTORE_ADAPTER, 0)
AC_SUBST(FILESTORE_SSL_INCPATH)
AC_SUBST(FILESTORE_SSL_LIBPATH)
AC_SUBST(FILESTORE_CPUTYPE, `fgrep Hardware /proc/cpuinfo | head -1 | awk '{ print $3 }'`)
AC_SUBST(FILESTORE_DEBUG)
AC_SUBST(FILESTORE_WITHOPENSSL)
AC_SUBST(FILESTORE_WITHIPV6)
AC_SUBST(OPENSSL_CA_DIR)
AC_SUBST(OPENSSL_SERVER_KEY)
AC_SUBST(OPENSSL_SERVER_CERT)
AC_SUBST(OPENSSL_CLIENT_KEY)
AC_SUBST(OPENSSL_CLIENT_CERT)
AC_SUBST(OPENSSL_SERVER_CMD, "openssl req -x509 -${OPENSSL_HASH} -newkey rsa:${OPENSSL_KEYSIZE} -days ${OPENSSL_EXPIRE} -nodes -subj \"${OPENSSL_SUBJ}\" -keyout ${OPENSSL_CA_DIR}/${OPENSSL_SERVER_KEY} -out ${OPENSSL_CA_DIR}/${OPENSSL_SERVER_CERT}")
AC_SUBST(OPENSSL_CLIENT_CMD, "openssl req -x509 -${OPENSSL_HASH} -newkey rsa:${OPENSSL_KEYSIZE} -days ${OPENSSL_EXPIRE} -nodes -subj \"${OPENSSL_SUBJ}\" -keyout ${OPENSSL_CA_DIR}/${OPENSSL_CLIENT_KEY} -out ${OPENSSL_CA_DIR}/${OPENSSL_CLIENT_CERT}")
# Switch to EC curve x25519 once OpenSSL 1.1.0 is released
#AC_SUBST(OPENSSL_SERVER_CMD, "openssl req -x509 -${OPENSSL_HASH} -newkey ec -pkeyopt ec_paramgen_curve:${OPENSSL_ECDHCURVE} -days ${OPENSSL_EXPIRE} -nodes -subj \"${OPENSSL_SUBJ}\" -keyout ${OPENSSL_CA_DIR}/${OPENSSL_SERVER_KEY} -out ${OPENSSL_CA_DIR}/${OPENSSL_SERVER_CERT}")
#AC_SUBST(OPENSSL_CLIENT_CMD, "openssl req -x509 -${OPENSSL_HASH} -newkey ec -pkeyopt ec_paramgen_curve:${OPENSSL_ECDHCURVE} -days ${OPENSSL_EXPIRE} -nodes -subj \"${OPENSSL_SUBJ}\" -keyout ${OPENSSL_CA_DIR}/${OPENSSL_CLIENT_KEY} -out ${OPENSSL_CA_DIR}/${OPENSSL_CLIENT_CERT}")

#AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES(
	[
		Makefile
		config.h
	]
)
AC_OUTPUT




