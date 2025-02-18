/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <folly/Benchmark.h>
#include <random>

#include "common/init/Init.h"

#include "fbpcf/engine/tuple_generator/oblivious_transfer/EmpShRandomCorrelatedObliviousTransferFactory.h"
#include "fbpcf/engine/tuple_generator/oblivious_transfer/ExtenderBasedRandomCorrelatedObliviousTransferFactory.h"
#include "fbpcf/engine/tuple_generator/oblivious_transfer/IBaseObliviousTransfer.h"
#include "fbpcf/engine/tuple_generator/oblivious_transfer/IRandomCorrelatedObliviousTransferFactory.h"
#include "fbpcf/engine/tuple_generator/oblivious_transfer/IknpShRandomCorrelatedObliviousTransferFactory.h"
#include "fbpcf/engine/tuple_generator/oblivious_transfer/NpBaseObliviousTransferFactory.h"
#include "fbpcf/engine/tuple_generator/oblivious_transfer/ferret/RcotExtenderFactory.h"
#include "fbpcf/engine/tuple_generator/oblivious_transfer/ferret/RegularErrorMultiPointCot.h"
#include "fbpcf/engine/tuple_generator/oblivious_transfer/ferret/RegularErrorMultiPointCotFactory.h"
#include "fbpcf/engine/tuple_generator/oblivious_transfer/ferret/SinglePointCotFactory.h"
#include "fbpcf/engine/tuple_generator/oblivious_transfer/ferret/TenLocalLinearMatrixMultiplierFactory.h"
#include "fbpcf/engine/util/test/benchmarks/BenchmarkHelper.h"
#include "fbpcf/engine/util/test/benchmarks/NetworkedBenchmark.h"
#include "fbpcf/engine/util/util.h"

namespace fbpcf::engine::tuple_generator::oblivious_transfer {

class NpBaseObliviousTransferBenchmark : public util::NetworkedBenchmark {
 public:
  void setup() override {
    auto [agent0, agent1] = util::getSocketAgents();

    NpBaseObliviousTransferFactory factory;
    sender_ = factory.create(std::move(agent0));
    receiver_ = factory.create(std::move(agent1));

    std::random_device rd;
    std::mt19937_64 e(rd());
    std::uniform_int_distribution<uint8_t> randomChoice(0, 1);

    choice_ = std::vector<bool>(size_);
    for (auto i = 0; i < size_; ++i) {
      choice_[i] = randomChoice(e);
    }
  }

  void runSender() override {
    sender_->send(size_);
  }

  void runReceiver() override {
    receiver_->receive(choice_);
  }

  std::pair<uint64_t, uint64_t> getTrafficStatistics() override {
    return sender_->getTrafficStatistics();
  }

 private:
  size_t size_ = 1024;

  std::unique_ptr<IBaseObliviousTransfer> sender_;
  std::unique_ptr<IBaseObliviousTransfer> receiver_;

  std::vector<bool> choice_;
};

BENCHMARK_COUNTERS(NpBaseObliviousTransfer, counters) {
  NpBaseObliviousTransferBenchmark benchmark;
  benchmark.runBenchmark(counters);
}

class RandomCorrelatedObliviousTransferBenchmark
    : public util::NetworkedBenchmark {
 public:
  void setup() override {
    auto [agent0, agent1] = util::getSocketAgents();
    agent0_ = std::move(agent0);
    agent1_ = std::move(agent1);

    std::random_device rd;
    std::mt19937_64 e(rd());
    std::uniform_int_distribution<uint64_t> dist(0, 0xFFFFFFFFFFFFFFFF);
    delta_ = _mm_set_epi64x(dist(e), dist(e));
    util::setLsbTo1(delta_);
  }

  void runSender() override {
    sender_ = factory_->create(delta_, std::move(agent0_));
    sender_->rcot(size_);
  }

  void runReceiver() override {
    receiver_ = factory_->create(std::move(agent1_));
    receiver_->rcot(size_);
  }

  std::pair<uint64_t, uint64_t> getTrafficStatistics() override {
    return sender_->getTrafficStatistics();
  }

 protected:
  std::unique_ptr<IRandomCorrelatedObliviousTransferFactory> factory_;

 private:
  size_t size_ = 1000000;
  __m128i delta_;

  std::unique_ptr<communication::IPartyCommunicationAgent> agent0_;
  std::unique_ptr<communication::IPartyCommunicationAgent> agent1_;

  std::unique_ptr<IRandomCorrelatedObliviousTransfer> sender_;
  std::unique_ptr<IRandomCorrelatedObliviousTransfer> receiver_;
};

class EmpShRandomCorrelatedObliviousTransferBenchmark final
    : public RandomCorrelatedObliviousTransferBenchmark {
 public:
  void setup() override {
    RandomCorrelatedObliviousTransferBenchmark::setup();
    factory_ = std::make_unique<EmpShRandomCorrelatedObliviousTransferFactory>(
        std::make_unique<util::AesPrgFactory>());
  }
};

BENCHMARK_COUNTERS(EmpShRandomCorrelatedObliviousTransfer, counters) {
  EmpShRandomCorrelatedObliviousTransferBenchmark benchmark;
  benchmark.runBenchmark(counters);
}

class IknpShRandomCorrelatedObliviousTransferBenchmark final
    : public RandomCorrelatedObliviousTransferBenchmark {
 public:
  void setup() override {
    RandomCorrelatedObliviousTransferBenchmark::setup();
    factory_ = std::make_unique<IknpShRandomCorrelatedObliviousTransferFactory>(
        std::make_unique<NpBaseObliviousTransferFactory>());
  }
};

BENCHMARK_COUNTERS(IknpShRandomCorrelatedObliviousTransfer, counters) {
  IknpShRandomCorrelatedObliviousTransferBenchmark benchmark;
  benchmark.runBenchmark(counters);
}

class ExtenderBasedRandomCorrelatedObliviousTransferWithIknpBenchmark final
    : public RandomCorrelatedObliviousTransferBenchmark {
 public:
  void setup() override {
    RandomCorrelatedObliviousTransferBenchmark::setup();
    factory_ = std::make_unique<
        ExtenderBasedRandomCorrelatedObliviousTransferFactory>(
        std::make_unique<IknpShRandomCorrelatedObliviousTransferFactory>(
            std::make_unique<NpBaseObliviousTransferFactory>()),
        std::make_unique<ferret::RcotExtenderFactory>(
            std::make_unique<ferret::TenLocalLinearMatrixMultiplierFactory>(),
            std::make_unique<ferret::RegularErrorMultiPointCotFactory>(
                std::make_unique<ferret::SinglePointCotFactory>())),
        ferret::kExtendedSize,
        ferret::kBaseSize,
        ferret::kWeight);
  }
};

BENCHMARK_COUNTERS(
    ExtenderBasedRandomCorrelatedObliviousTransferWithIknp,
    counters) {
  ExtenderBasedRandomCorrelatedObliviousTransferWithIknpBenchmark benchmark;
  benchmark.runBenchmark(counters);
}

class ExtenderBasedRandomCorrelatedObliviousTransferWithEmpBenchmark final
    : public RandomCorrelatedObliviousTransferBenchmark {
 public:
  void setup() override {
    RandomCorrelatedObliviousTransferBenchmark::setup();
    factory_ = std::make_unique<
        ExtenderBasedRandomCorrelatedObliviousTransferFactory>(
        std::make_unique<EmpShRandomCorrelatedObliviousTransferFactory>(
            std::make_unique<util::AesPrgFactory>()),
        std::make_unique<ferret::RcotExtenderFactory>(
            std::make_unique<ferret::TenLocalLinearMatrixMultiplierFactory>(),
            std::make_unique<ferret::RegularErrorMultiPointCotFactory>(
                std::make_unique<ferret::SinglePointCotFactory>())),
        ferret::kExtendedSize,
        ferret::kBaseSize,
        ferret::kWeight);
  }
};

BENCHMARK_COUNTERS(
    ExtenderBasedRandomCorrelatedObliviousTransferWithEmp,
    counters) {
  ExtenderBasedRandomCorrelatedObliviousTransferWithEmpBenchmark benchmark;
  benchmark.runBenchmark(counters);
}

} // namespace fbpcf::engine::tuple_generator::oblivious_transfer

int main(int argc, char* argv[]) {
  facebook::initFacebook(&argc, &argv);
  folly::runBenchmarks();
  return 0;
}
