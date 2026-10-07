#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include <vector>
#include "point.hpp"
#include "centroid.hpp"

class Simulator {
  public:
    Simulator();

    void Reset();
    void LoadFromFile();
    
  private:
    std::vector<Point> points_;
    std::vector<Centroid> centroids_;
};

#endif