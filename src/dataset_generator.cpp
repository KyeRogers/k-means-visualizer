#include "dataset_generator.hpp"

DatasetGenerator::DatasetGenerator(const int kWorldMin, const int kWorldMax)
    : seed_{42},
      rng_{42},
      points_(0),
      type_{DatasetType::random},
      point_count_{0},
      blob_count_{0},
      standard_deviation_{30},
      kWorldMin_{kWorldMin},
      KWorldMax_{kWorldMax} {}

void DatasetGenerator::SetDataset(const int seed, std::mt19937 rng,
                                  DatasetType type, const int points,
                                  const int blob_count,
                                  const int standard_deviation) {
  seed_ = seed;
  rng_ = rng;
  type_ = type;
  point_count_ = points;
  blob_count_ = blob_count;
  standard_deviation_ = standard_deviation;
}

std::vector<Point> DatasetGenerator::Generate() {
  points_.clear();
  if (type_ == DatasetType::random) {
    GenerateRandom();
  } else if (type_ == DatasetType::blobs) {
    GenerateBlobs();
  } else if (type_ == DatasetType::elongated_clusters) {
    GenerateElongatedClusters();
  } else if (type_ == DatasetType::ring) {
    GenerateRing();
  }
  return points_;
}

void DatasetGenerator::GenerateRandom() {
  for (int i{0}; i < point_count_; i++) {
    points_.push_back(Point(GenerateRandomDouble(), GenerateRandomDouble(), 0));
  }
}

void DatasetGenerator::GenerateBlobs() {
  std::vector<Point> blobs;
  // generate blob centres
  for (int i{0}; i < blob_count_; i++) {
    blobs.push_back(Point(GenerateRandomDouble(), GenerateRandomDouble(), 0));
  }
  int blob_choice{0};
  std::uniform_int_distribution dist(0, blob_count_ - 1);
  for (int i{0}; i < point_count_; i++) {
    // choose a blob
    blob_choice = dist(rng_);
    double x_coordenate, y_coordenate;

    // generate offsets and add to point until its valid
    do {
      x_coordenate = blobs[blob_choice].GetX() + GenerateGaussianOffset();
      y_coordenate = blobs[blob_choice].GetY() + GenerateGaussianOffset();
    } while (!IsValidPoint(x_coordenate, y_coordenate));

    points_.push_back(Point(x_coordenate, y_coordenate, 0));
  }
}

double DatasetGenerator::GenerateRandomDouble() {
  std::uniform_real_distribution<double> dist(kWorldMin_, KWorldMax_);
  return dist(rng_);
}

double DatasetGenerator::GenerateGaussianOffset() {
  std::normal_distribution<double> gaussian(0.0, standard_deviation_);
  return gaussian(rng_);
}

bool DatasetGenerator::IsValidPoint(const double x, const double y) const {
  return (x > kWorldMin_ && x < KWorldMax_ && y > kWorldMin_ && y < KWorldMax_);
}
