noinst_PROGRAMS = benchmark

benchmark_SOURCES = main.cc factory.benchmark.cc

AM_CPPFLAGS = -I $(srcdir)/.. -I $(builddir)/..

# Google Benchmark and its dependencies
LDADD = -lbenchmark -lpthread -lrt

