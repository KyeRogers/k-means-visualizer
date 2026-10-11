#include "dataset_generator.hpp"

DatasetGenerator::DatasetGenerator()
    : seed_{42},
      rng_{42},
      points_(0),
      type_{DatasetType::random},
      blob_count_{0} {}

void DatasetGenerator::SetDataset(const int seed, std::mt19937 rng, DatasetType type,
                const int blob_count) {
                  seed_ = seed;
                  rng_ = rng;
                  type_ = type;
                  blob_count_ = blob_count;
                }
