# Copyright (c) 2006, Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


AC_PREREQ(2.57)

AC_INIT(breakpad, 0.1, google-breakpad-dev@googlegroups.com)
dnl Sanity check: the argument is just a file that should exist.
AC_CONFIG_SRCDIR(README)
AC_CONFIG_AUX_DIR(autotools)
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE(subdir-objects tar-ustar 1.11.1)
AM_CONFIG_HEADER(src/config.h)

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_RANLIB

AC_HEADER_STDC
m4_include(m4/ax_pthread.m4)
AX_PTHREAD
AC_CHECK_HEADERS([a.out.h])

# Only build Linux client libs when compiling for Linux
case $host in
     *-*-linux* | *-android* )
        LINUX_HOST=true
        ;;
esac
AM_CONDITIONAL(LINUX_HOST, test x$LINUX_HOST = xtrue)

AC_ARG_ENABLE(m32,
              AS_HELP_STRING([--enable-m32],
                             [Compile/build with -m32]
                             [(default is no)]),
              [case "${enableval}" in
                 yes)
                   CFLAGS="${CFLAGS} -m32"
                   CXXFLAGS="${CXXFLAGS} -m32"
                   usem32=true
                   ;;
                 no)
                   usem32=false
                   ;;
                 *)
                   AC_MSG_ERROR(bad value ${enableval} for --enable-m32)
                   ;;
               esac],
              [usem32=false])

AC_ARG_ENABLE(processor,
              AS_HELP_STRING([--disable-processor],
                             [Don't build processor library]
                             [(default is no)]),
              [case "${enableval}" in
                 yes)
                   disable_processor=false
                   ;;
                 no)
                   disable_processor=true
                   ;;
                 *)
                   AC_MSG_ERROR(bad value ${enableval} for --disable-processor)
                   ;;
               esac],
              [disable_processor=false])
AM_CONDITIONAL(DISABLE_PROCESSOR, test x$disable_processor = xtrue)

AC_ARG_ENABLE(tools,
              AS_HELP_STRING([--disable-tools],
                             [Don't build tool binaries]
                             [(default is no)]),
              [case "${enableval}" in
                 yes)
                   disable_tools=false
                   ;;
                 no)
                   disable_tools=true
                   ;;
                 *)
                   AC_MSG_ERROR(bad value ${enableval} for --disable-tools)
                   ;;
               esac],
              [disable_tools=false])
AM_CONDITIONAL(DISABLE_TOOLS, test x$disable_tools = xtrue)

if test x$LINUX_HOST = xfalse -a x$disable_processor = xtrue -a x$disable_tools = xtrue; then
   AC_MSG_ERROR([--disable-processor and --disable-tools were specified, and not building for Linux. Nothing to build!])
fi

AC_ARG_ENABLE(selftest,
              AS_HELP_STRING([--enable-selftest],
                             [Run extra tests with "make check" ]
                             [(may conflict with optimizations) ]
                             [(default is no)]),
              [case "${enableval}" in
                 yes)
                   selftest=true
                   ;;
                 no)
                   selftest=false
                   ;;
                 *)
                   AC_MSG_ERROR(bad value ${enableval} for --enable-selftest)
                   ;;
               esac],
              [selftest=false])
AM_CONDITIONAL(SELFTEST, test x$selftest = xtrue)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
