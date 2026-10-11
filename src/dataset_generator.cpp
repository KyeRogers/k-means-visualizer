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
      standard_deviation_x_{30},
      standard_deviation_y_{30},
      ring_radius_{100},
      kWorldMin_{kWorldMin},
      KWorldMax_{kWorldMax} {}

void DatasetGenerator::SetDataset(const int seed, std::mt19937 rng,
                                  DatasetType type, const int points,
                                  const int blob_count,
                                  const int standard_deviation_x,
                                  const int standard_deviation_y,
                                  const int ring_radius) {
  seed_ = seed;
  rng_ = rng;
  type_ = type;
  point_count_ = points;
  blob_count_ = blob_count;
  standard_deviation_x_ = standard_deviation_x;
  standard_deviation_y_ = standard_deviation_y;
  ring_radius_ = ring_radius;
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

double DatasetGenerator::GenerateAngle() {
  std::uniform_real_distribution<double> rotation_dist(0.0, M_PI);
  return rotation_dist(rng_);
}

void DatasetGenerator::GenerateBlobs() {
  std::vector<Blob> blobs;
  // generate blob centres
  for (int i{0}; i < blob_count_; i++) {
    blobs.push_back(Blob{GenerateRandomDouble(), GenerateRandomDouble(), 0.0});
  }
  int blob_choice{0};
  std::uniform_int_distribution dist(0, blob_count_ - 1);
  for (int i{0}; i < point_count_; i++) {
    // choose a blob
    blob_choice = dist(rng_);
    double x_coordenate, y_coordenate;

    // generate offsets and add to point until its valid
    do {
      x_coordenate =
          blobs[blob_choice].x + GenerateGaussianOffset(standard_deviation_x_);
      y_coordenate =
          blobs[blob_choice].y + GenerateGaussianOffset(standard_deviation_x_);
    } while (!IsValidPoint(x_coordenate, y_coordenate));

    points_.push_back(Point(x_coordenate, y_coordenate, 0));
  }
}

double DatasetGenerator::GenerateRandomDouble() {
  std::uniform_real_distribution<double> dist(kWorldMin_, KWorldMax_);
  return dist(rng_);
}

double DatasetGenerator::GenerateGaussianOffset(const int standard_deviation) {
  std::normal_distribution<double> gaussian(0.0, standard_deviation);
  return gaussian(rng_);
}

bool DatasetGenerator::IsValidPoint(const double x, const double y) const {
  return (x > kWorldMin_ && x < KWorldMax_ && y > kWorldMin_ && y < KWorldMax_);
}

void DatasetGenerator::GenerateElongatedClusters() {
  std::vector<Blob> blobs;
  blobs.reserve(blob_count_);

  // Generate blob centers and random rotations.

  for (int i = 0; i < blob_count_; ++i) {
    Blob blob{GenerateRandomDouble(), GenerateRandomDouble(), GenerateAngle()};

    blobs.push_back(blob);
  }

  // Precompute rotation values for each blob.
  std::vector<std::pair<double, double>> rotations;
  rotations.reserve(blob_count_);

  for (const Blob& blob : blobs) {
    rotations.emplace_back(std::cos(blob.rotation), std::sin(blob.rotation));
  }

  // Choose a blob for each point.
  std::uniform_int_distribution<int> blob_dist(0, blob_count_ - 1);

  for (int i = 0; i < point_count_; ++i) {
    const int blob_index = blob_dist(rng_);
    const Blob& blob = blobs[blob_index];

    const double cos_rotation = rotations[blob_index].first;
    const double sin_rotation = rotations[blob_index].second;

    double x_coordinate;
    double y_coordinate;

    // Generate, rotate, and validate the point.
    do {
      const double x_offset = GenerateGaussianOffset(standard_deviation_x_);
      const double y_offset = GenerateGaussianOffset(standard_deviation_y_);

      const double rotated_x_offset =
          x_offset * cos_rotation - y_offset * sin_rotation;
      const double rotated_y_offset =
          x_offset * sin_rotation + y_offset * cos_rotation;

      x_coordinate = blob.x + rotated_x_offset;
      y_coordinate = blob.y + rotated_y_offset;

    } while (!IsValidPoint(x_coordinate, y_coordinate));

    points_.emplace_back(x_coordinate, y_coordinate, 0);
  }
}

void DatasetGenerator::GenerateRing() {
  const double centre_x{500.0}, centre_y{500.0};

  double x_coordenate, y_coordenate;

  for (int i{0}; i < point_count_; i++) {
    double angle = GenerateAngle() * 2;
    const double cos_rotation = cos(angle);
    const double sin_rotation = sin(angle);
    do {
      double offset = GenerateGaussianOffset(standard_deviation_x_);
      double radius = ring_radius_ + offset;
      x_coordenate = centre_x + radius * cos_rotation;
      y_coordenate = centre_y + radius * sin_rotation;
    } while (!IsValidPoint(x_coordenate, y_coordenate));

    points_.emplace_back(Point(x_coordenate, y_coordenate, 0));
  }
}

void DatasetGenerator::GenerateTwoMoons() {
  double centre_x_1{500.0 - (ring_radius_ / 2)};
  double centre_x_2{500.0 + (ring_radius_ / 2)};
  double y_centre{500.0};


  std::uniform_int_distribution moon_dist(0, 1);

  double x_coordenate, y_coordenate;
  for (int i{0}; i < point_count_; i++) {
    // choose which moon
    double angle = GenerateAngle();
    do {
      double offset = GenerateGaussianOffset(standard_deviation_x_);
      double radius = ring_radius_ + offset;
      if (moon_dist(rng_) == 0) {
        // second moon
        x_coordenate = centre_x_1 + radius * cos(angle);
      } else {
        angle = -angle;
        x_coordenate = centre_x_2 + radius * cos(angle);
        
      }
      y_coordenate = y_centre + radius * sin(angle);
    } while (!IsValidPoint(x_coordenate, y_coordenate));
    points_.emplace_back(Point(x_coordenate, y_coordenate, 0));
  }
}
