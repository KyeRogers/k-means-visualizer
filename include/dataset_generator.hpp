#ifndef DATASET_GENERATOR_HPP
#define DATASET_GENERATOR_HPP

#include <random>
#include <vector>

#include "point.hpp"

enum DatasetType { random, blobs, elongated_clusters, two_moons, ring };

class DatasetGenerator {
 public:
  DatasetGenerator(const int kWorldMin, const int kWorldMax);

  void SetDataset(const int seed, std::mt19937 rng, DatasetType type,
                  const int points, const int blob_count,
                  const int standard_deviation_);
  std::vector<Point> Generate();

 private:
  int seed_{42};
  std::mt19937 rng_{42};
  std::vector<Point> points_;
  DatasetType type_;
  int point_count_;
  int blob_count_;
  int standard_deviation_;
  int kWorldMin_;
  int KWorldMax_;

  double GenerateRandomDouble();
  double GenerateGaussianOffset();
  bool IsValidPoint(const double x, const double y) const;

  void GenerateRandom();
  void GenerateBlobs();
  void GenerateElongatedClusters();
  void GenerateRing();
};

#endif