dnl Process this file with autoconf to produce a configure script.

dnl We need at least autoconf 2.69 for this configure.ac to work.
AC_PREREQ([2.69])
AC_INIT([unique-factory], [0.2.0], [julian.rueth@fsfe.org])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([1.14.1 subdir-objects foreign])

AC_CANONICAL_HOST

dnl Find C++ Compiler
AC_PROG_CXX

dnl Set INSTALL or use install-sh.
AC_PROG_INSTALL

dnl We build our tests with libtool.
LT_INIT

dnl We run the valgrind test-suite in make check-valgrind (if valgrind is installed)
AX_VALGRIND_DFLT([sgcheck], [off])
AX_VALGRIND_DFLT([drd], [off])
AX_VALGRIND_DFLT([helgrind], [off])
AX_VALGRIND_CHECK()

dnl Check for required libraries
AC_LANG([C++])

dnl We use some C++17 features, such as try_emplace.
AX_CXX_COMPILE_STDCXX(17)

dnl Our benchmarks use Google's C++ benchmark library.
dnl We fail if they cannot be found but let the user disable it explicitly.
AC_ARG_WITH([benchmark], AS_HELP_STRING([--without-benchmark], [Do not build C++ benchmarks that require google/benchmark]))
AS_IF([test "x$with_benchmark" != "xno"],
      [
       with_benchmark=yes
       AC_CHECK_HEADERS([benchmark/benchmark.h], , AC_MSG_ERROR([benchmark headers not found; run --without-benchmark to disable building of benchmark/]))
      ], [])
AM_CONDITIONAL([HAVE_BENCHMARK], [test "x$with_benchmark" = "xyes"])

AC_CONFIG_FILES([Makefile test/Makefile benchmark/Makefile])

AC_OUTPUT
