#include "simulator.hpp"

#include <fstream>
#include <sstream>

void CentroidCache::Update(const double x, const double y) {
  sum_x += x;
  sum_y += y;
  count++;
}

double CentroidCache::NewX() { return sum_x / count; }
double CentroidCache::NewY() { return sum_y / count; }

Simulator::Simulator() : points_(0), centroids_(0), current_cost_{0} {};

void Simulator::Reset() {
  points_.erase(points_.begin(), points_.end());
  centroids_.erase(centroids_.begin(), centroids_.end());
}

void Simulator::LoadFromFile(const std::string& filename) {
  std::ifstream input_file(filename);
  if (!input_file) {
    throw std::runtime_error("Could not open data file: " + filename);
  }

  int temp_x, temp_y;
  std::string line;
  while (std::getline(input_file, line))
  {
    std::stringstream ss(line);
    ss >> temp_x >> temp_y;
    points_.push_back(Point(temp_x, temp_y, 0));
  }
}

void Simulator::KMeansIteration() {
  // asign each point to the nearest centroid
  current_cost_ = 0;
  for (Point& point : points_) {
    current_cost_ += ClosestCentroid(point);
  }

  // update centroids
  std::vector<CentroidCache> temp_data(centroids_.size());
  for (Point& point : points_) {
    temp_data[point.GetCentroid() - 1].Update(point.GetX(), point.GetY());
  }


  
}

/** @brief returns the squared euclidian distance between two points*/
double Simulator::SquaredEuclidianDistance(const double x1, const double y1, const double x2, const double y2) const {
  return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
}

/** @brief sets the point to the correct centroid, returns the distance cost */
double Simulator::ClosestCentroid(Point& point) {
  double min_distance_{__DBL_MAX__};
  int index{1};
  for (int i{0}; i < centroids_.size(); i++) {

    double temp = SquaredEuclidianDistance(centroids_[i].GetX(), centroids_[i].GetY(), point.GetX(), point.GetY());
    if (temp < min_distance_) {
      temp = min_distance_;
      index = i + 1;
    }
  }
  point.SetCentroid(index);
  return min_distance_;
}