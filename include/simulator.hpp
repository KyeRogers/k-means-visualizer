#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include <vector>
#include <string>
#include "point.hpp"
#include "centroid.hpp"


struct CentroidCache {
  double sum_x{0.0};
  double sum_y{0.0};
  double count{0.0};

  void Update(const double x, const double y);
  double NewX();
  double NewY();
};

class Simulator {
  public:
    Simulator();

    void KMeansIteration();
    
    
    void Reset();
    void LoadFromFile(const std::string& filename);
    
    private:
    std::vector<Point> points_;
    std::vector<Centroid> centroids_;
    double current_cost_;

    double SquaredEuclidianDistance(const double x1, const double y1, const double x2, const double y2) const;
    double ClosestCentroid(Point& point);
};

#endif