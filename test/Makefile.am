check_PROGRAMS = factory

TESTS = $(check_PROGRAMS)

factory_SOURCES = factory.test.cc main.cc

# We vendor the single-header catch2 to run our tests
EXTRA_DIST = external/catch2/single_include/catch2/catch.hpp

@VALGRIND_CHECK_RULES@
