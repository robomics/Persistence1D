#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include "persistence1d/persistence1d.hpp"

using namespace p1d;

void SecondCallOnEmptyData() {
  Persistence1D p;
  std::vector<TPairedExtrema> pairs;
  std::vector<std::int64_t> min, max;
  std::vector<double> data1, data2;

  data1.push_back(1.0);
  data1.push_back(2.0);
  data1.push_back(3.0);
  data1.push_back(1.0);

  p.RunPersistence(data1);
  p.GetExtremaIndices(min, max);

  p.RunPersistence(data2);
  p.GetExtremaIndices(min, max);
  p.GetPairedExtrema(pairs);
  double minVal = p.GetGlobalMinimumValue();
  std::int64_t idx = p.GetGlobalMinimumIndex();

  assert(minVal == 0);
  assert(idx == -1);
  assert(min.empty());
  assert(max.empty());
  assert(pairs.empty());

  assert(p.VerifyResults());
  std::cout << "SecondCallOnEmptyData: passed\n";
}
void MutliCallPersistence() {
  Persistence1D p;
  std::vector<TPairedExtrema> pairs;
  std::vector<std::int64_t> min, max;
  std::vector<double> data1, data2;

  data1.push_back(1.0);
  data1.push_back(2.0);
  data1.push_back(3.0);
  data1.push_back(1.0);

  data2 = std::vector<double>(data1);
  data2.push_back(4.0);
  data2.push_back(10.0);
  data2.push_back(-5.0);

  p.RunPersistence(data1);
  p.GetExtremaIndices(min, max);
  p.GetExtremaIndices(min, max);
  p.GetExtremaIndices(min, max);

  assert(min.size() == 1);
  assert(max.size() == 1);

  p.GetPairedExtrema(pairs);
  p.GetPairedExtrema(pairs);
  assert(pairs.size() == 1);

  assert(p.GetGlobalMinimumValue() != 0);
  assert(p.GetGlobalMinimumIndex() != -1);

  assert(p.VerifyResults());

  p.RunPersistence(data2);
  p.GetExtremaIndices(min, max);
  p.GetPairedExtrema(pairs);
  assert(p.GetGlobalMinimumValue() == -5.0);
  assert(p.GetGlobalMinimumIndex() == 6);
  assert(min.size() == 2);
  assert(max.size() == 2);
  assert(pairs.size() == 2);

  // now check the filters:
  p.GetExtremaIndices(min, max, 10);
  p.GetPairedExtrema(pairs, 2.1);
  assert(p.GetGlobalMinimumValue() == -5.0);
  assert(p.GetGlobalMinimumIndex() == 6);
  assert(min.empty());
  assert(max.empty());
  assert(pairs.size() == 1);

  assert(p.VerifyResults());

  std::cout << "MutliCallPersistence: passed\n";
}
void RunOnEmptyData() {
  Persistence1D p;
  std::vector<TPairedExtrema> pairs;
  std::vector<std::int64_t> min, max;
  std::vector<double> data;

  p.RunPersistence(data);

  p.GetExtremaIndices(min, max);
  assert(min.empty());
  assert(max.empty());

  p.GetExtremaIndices(min, max, 9.0, true);
  assert(min.empty());
  assert(max.empty());

  assert(p.GetGlobalMinimumIndex() == -1);
  assert(p.GetGlobalMinimumValue() == 0);

  p.GetPairedExtrema(pairs);
  assert(pairs.empty());

  p.GetPairedExtrema(pairs, 15.0, true);
  assert(pairs.empty());

  assert(p.VerifyResults());

  std::cout << "RunOnEmptyData: passed\n";
}
void CallsBeforeRuns() {
  Persistence1D p;
  std::vector<TPairedExtrema> pairs;
  std::vector<std::int64_t> min, max;
  std::vector<double> data;

  p.GetExtremaIndices(min, max);
  assert(min.empty());
  assert(max.empty());

  p.GetExtremaIndices(min, max, 9.0, true);
  assert(min.empty());
  assert(max.empty());

  assert(p.GetGlobalMinimumIndex() == -1);
  assert(p.GetGlobalMinimumValue() == 0);

  p.GetPairedExtrema(pairs);
  assert(pairs.empty());

  p.GetPairedExtrema(pairs, 15.0, true);
  assert(pairs.empty());

  assert(p.VerifyResults());

  std::cout << "CallsBeforeRuns: passed\n";
}
void TestInputSizeOne() {
  Persistence1D p;
  std::vector<TPairedExtrema> pairs;
  std::vector<std::int64_t> min, max;

  std::vector<double> data;
  data.push_back(10.0);

  p.RunPersistence(data);
  p.GetPairedExtrema(pairs);
  p.GetExtremaIndices(min, max);
  std::int64_t minIdx = p.GetGlobalMinimumIndex();
  double minVal = p.GetGlobalMinimumValue();

  assert(pairs.empty() && min.empty() && max.empty());
  assert(minIdx == 0);
  assert(minVal != -1);

  assert(p.VerifyResults());

  std::cout << "TestInputSizeOne: passed\n";
}
void TestInputSizeTwo() {
  Persistence1D p;
  std::vector<TPairedExtrema> pairs;
  std::vector<std::int64_t> min, max;

  std::vector<double> data;
  data.push_back(10.0);
  data.push_back(20.0);

  p.RunPersistence(data);
  p.GetPairedExtrema(pairs);
  p.GetExtremaIndices(min, max);
  std::int64_t minIdx = p.GetGlobalMinimumIndex();
  double minVal = p.GetGlobalMinimumValue();

  assert(pairs.empty() && min.empty() && max.empty());
  assert(minIdx != -1);
  assert(minVal != 0);

  assert(p.VerifyResults());

  std::cout << "TestInputSizeTwo: passed\n";
}
void RandomizedTesting() {
  std::vector<double> data;
  std::int64_t size = rand() % 10000;
  data.reserve(size);

  Persistence1D p;

  // create data
  for (std::int64_t i = 0; i < size; i++) {
    data.push_back((double)rand());
  }

  p.RunPersistence(data);
  assert(p.VerifyResults());
}

int main() {
  TestInputSizeOne();
  TestInputSizeTwo();
  RunOnEmptyData();
  CallsBeforeRuns();
  MutliCallPersistence();
  SecondCallOnEmptyData();
  for (std::size_t i = 0; i < 100; i++) {
    RandomizedTesting();
  }
  return 0;
}
