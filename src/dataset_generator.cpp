#include "dataset_generator.hpp"

#include <algorithm>
#include <cmath>

DatasetGenerator::DatasetGenerator(const int kWorldMin, const int kWorldMax)
    : seed_{42},
      rng_{42},
      points_(0),
      type_{DatasetType::Random},
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
  if (type_ == DatasetType::Random) {
    GenerateRandom();
  } else if (type_ == DatasetType::Blobs) {
    GenerateBlobs();
  } else if (type_ == DatasetType::ElongatedClusters) {
    GenerateElongatedClusters();
  } else if (type_ == DatasetType::TwoMoons) {
    GenerateTwoMoons();
  } else if (type_ == DatasetType::Ring) {
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

void DatasetGenerator::GenerateElongatedClusters() {
  std::normal_distribution<double> offset(0.0, standard_deviation_);
  for (int i = 0; i < point_count_; ++i) {
    const double centre = i < point_count_ / 2 ? kWorldMin_ + 300.0
                                               : kWorldMin_ + 700.0;
    const double x = std::clamp(centre + offset(rng_) * 2.5, 
                                static_cast<double>(kWorldMin_ + 1),
                                static_cast<double>(KWorldMax_ - 1));
    const double y = std::clamp(kWorldMin_ + 500.0 + offset(rng_),
                                static_cast<double>(kWorldMin_ + 1),
                                static_cast<double>(KWorldMax_ - 1));
    points_.emplace_back(x, y, 0);
  }
}

void DatasetGenerator::GenerateRing() {
  constexpr double kPi = 3.14159265358979323846;
  std::uniform_real_distribution<double> angle(0.0, 2.0 * kPi);
  std::normal_distribution<double> radius(280.0, standard_deviation_ * 0.25);
  for (int i = 0; i < point_count_; ++i) {
    const double r = radius(rng_);
    const double a = angle(rng_);
    points_.emplace_back(
        std::clamp(kWorldMin_ + 500.0 + r * std::cos(a),
                   static_cast<double>(kWorldMin_ + 1),
                   static_cast<double>(KWorldMax_ - 1)),
        std::clamp(kWorldMin_ + 500.0 + r * std::sin(a),
                   static_cast<double>(kWorldMin_ + 1),
                   static_cast<double>(KWorldMax_ - 1)),
        0);
  }
}

void DatasetGenerator::GenerateTwoMoons() {
  constexpr double kPi = 3.14159265358979323846;
  std::uniform_real_distribution<double> angle(0.0, kPi);
  std::normal_distribution<double> noise(0.0, standard_deviation_ * 0.15);
  for (int i = 0; i < point_count_; ++i) {
    const double a = angle(rng_);
    const bool second = i >= point_count_ / 2;
    const double x = 500.0 + 280.0 * std::cos(a) + noise(rng_) +
                     (second ? 40.0 : -40.0);
    const double y = 500.0 + (second ? -280.0 * std::sin(a) : 280.0 * std::sin(a)) +
                     noise(rng_);
    points_.emplace_back(std::clamp(x, 1.0, 999.0),
                         std::clamp(y, 1.0, 999.0), 0);
  }
}
