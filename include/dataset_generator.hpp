#ifndef DATASET_GENERATOR_HPP
#define DATASET_GENERATOR_HPP

#include <random>
#include <vector>

#include "point.hpp"

struct Blob {
  double x;
  double y;
  double rotation;
};

enum class DatasetType { Random, Blobs, ElongatedClusters, TwoMoons, Ring };

class DatasetGenerator {
 public:
  DatasetGenerator(const int kWorldMin, const int kWorldMax);

  void SetDataset(const int seed, std::mt19937 rng, DatasetType type,
                  const int points, const int blob_count,
                  const int standard_deviation_x,
                  const int standard_deviation_y);
  std::vector<Point> Generate();

 private:
  int seed_{42};
  std::mt19937 rng_{42};
  std::vector<Point> points_;
  DatasetType type_;
  int point_count_;
  int blob_count_;
  int standard_deviation_x_;
  int standard_deviation_y_;
  int kWorldMin_;
  int KWorldMax_;

  double GenerateRandomDouble();
  double GenerateGaussianOffset(const int standard_deviation);
  bool IsValidPoint(const double x, const double y) const;

  void GenerateRandom();
  void GenerateBlobs();
  void GenerateElongatedClusters();
  void GenerateRing();
  void GenerateTwoMoons();
};

#endif