#ifndef DATASET_GENERATOR_HPP
#define DATASET_GENERATOR_HPP

#include <random>
#include <vector>

#include "point.hpp"

enum DatasetType {
  random,
  blobs,
  elongated_clusters,
  two_moons,
  ring
};

class DatasetGenerator {
 public:
  DatasetGenerator();

  void SetDataset(const int seed, std::mt19937 rng, DatasetType type, const int blob_count);

 private:
  int seed_{42};
  std::mt19937 rng_{42};
  std::vector<Point> points_;
  DatasetType type_;
  int blob_count_;

};

#endif