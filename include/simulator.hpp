#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include <string>
#include <vector>

#include "centroid.hpp"
#include "point.hpp"
#include "renderer.hpp"

constexpr double kWorldMin = 0.0;
constexpr double kWorldMax = 1000.0;

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
  void Run();
  void RunMultiple(const int total);

  void Reset(const bool complete_reset);
  void LoadFromFile(const std::string& filename);

 private:
  std::vector<Point> points_;
  std::vector<Centroid> centroids_;
  double current_cost_;
  Renderer renderer_;
  int iteration_;
  bool running_;
  int k_;
  bool converged_;

  double SquaredEuclidianDistance(const double x1, const double y1,
                                  const double x2, const double y2) const;
  double ClosestCentroid(Point& point);

  double GenerateRandomDouble();
  void GenerateCentroids();
};

#endif