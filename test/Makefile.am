if HAVE_GOOGLETEST
  check_PROGRAMS = factory
  TESTS = $(check_PROGRAMS)
endif

factory_SOURCES = factory.test.cc

@VALGRIND_CHECK_RULES@

AM_CPPFLAGS = -I $(srcdir)/../ -I $(builddir)/../
AM_LDFLAGS = -lgtest -lbenchmark -lpthread -lrt
