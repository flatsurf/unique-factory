/**********************************************************************
 *  This file is part of unique-factory.
 *
 *        Copyright (C) 2019 Julian Rüth
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *********************************************************************/

// Include this file once at the end of your .test.cc file to get a main that
// runs unit tests and benchmarks.

#include <benchmark/benchmark.h>
#include <gtest/gtest.h>
#include <vector>

using std::istream_iterator;
using std::istringstream;
using std::string;
using std::vector;

class AbortBenchmarksOnError : public ::testing::EmptyTestEventListener {
  virtual void OnTestPartResult(const ::testing::TestPartResult& test_part_result) {
    if (test_part_result.failed()) {
      throw std::logic_error("Benchmark aborted due to failing assertion.");
    }
  }
};

int main(int argc, char** argv) {
  vector<string> args;
  for (int i = 0; i < argc; i++) {
    args.push_back(argv[i]);
  }

  char* env_argv = std::getenv("EXACTREAL_CHECK");
  if (env_argv != nullptr) {
    auto iss = istringstream(env_argv);
    copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(args));
  }

  argc = int(args.size());
  argv = new char*[argc];
  for (int i = 0; i < argc; i++) {
    const char* arg = args[i].c_str();
    argv[i] = new char[strlen(arg) + 1];
    strcpy(argv[i], arg);
  }

  testing::InitGoogleTest(&argc, argv);

  int result = RUN_ALL_TESTS();

  if (result) {
    return result;
  }

  testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
  listeners.Append(new AbortBenchmarksOnError);

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();

  return result;
}
