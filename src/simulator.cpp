#include "simulator.hpp"

Simulator::Simulator() : points_(0), centroids_(0) {};

void Simulator::Reset() {
  points_.erase(points_.begin(), points_.end());
  centroids_.erase(centroids_.begin(), centroids_.end());

}