#include "simulator.hpp"

#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

void CentroidCache::Update(const double x, const double y) {
  sum_x += x;
  sum_y += y;
  count++;
}

double CentroidCache::NewX() { return sum_x / count; }
double CentroidCache::NewY() { return sum_y / count; }

Simulator::Simulator()
    : points_(0),
      centroids_(0),
      current_cost_{0},
      renderer_(),
      iteration_{0},
      running_{false},
      k_{0},
      converged_{false} {};

void Simulator::Reset(const bool complete_reset) {
  if (!complete_reset) {
    for (Point& point : points_) {
      point.SetCentroid(0);
    }
    GenerateCentroids();
  } else {
    points_.clear();
    centroids_.clear();
  }
  iteration_ = 0;
  converged_ = false;
}

void Simulator::LoadFromFile(const std::string& filename) {
  std::ifstream input_file(filename);
  if (!input_file) {
    throw std::runtime_error("Could not open data file: " + filename);
  }

  double temp_x, temp_y;
  std::string line;
  while (std::getline(input_file, line)) {
    std::stringstream ss(line);
    ss >> temp_x >> temp_y;
    points_.push_back(Point(temp_x, temp_y, 0));
  }
}

void Simulator::Run() {
  renderer_.Initialize(kWorldMin, kWorldMax);

  while (!renderer_.ShouldClose()) {
    // 1. Get user input
    Renderer::InputEvents events = renderer_.PollInput(converged_);
    // 2. Handle input

    if (events.k_selected) {
      k_ = events.k;
      GenerateCentroids();
    }

    if (events.load_file) {
      LoadFromFile(events.load_filename);
      iteration_ = 0;
    }

    if (events.multi_run_requested) {
      RunMultiple(events.total_runs);
    }

    if (events.main_menu) {
      renderer_.Initialize(kWorldMin, kWorldMax);
      Reset(true);
    }

    if (events.add_point) {
      points_.push_back(Point(events.point_x, events.point_y, 0));
    }

    if (events.step) {
      KMeansIteration();
    }

    if (events.toggle_running) {
      running_ = !running_;
    }

    if (events.reset) {
      Reset(false);
    }

    // 3. Automatically run K-means if enabled
    if (running_) {
      KMeansIteration();
    }

    // 4. Render the NEW state
    renderer_.Render(points_, centroids_, current_cost_, iteration_,
                     converged_);
  }
}
/** @brief returns true if succeded, false if converged  */
void Simulator::KMeansIteration() {
  iteration_++;
  bool changed{false};
  // asign each point to the nearest centroid
  current_cost_ = 0;
  for (Point& point : points_) {
    int temp{point.GetCentroid()};
    current_cost_ += ClosestCentroid(point);
    if (temp != point.GetCentroid()) {
      changed = true;
    }
  }

  // update centroids
  std::vector<CentroidCache> centroid_cache(centroids_.size());
  for (Point& point : points_) {
    centroid_cache[point.GetCentroid() - 1].Update(point.GetX(), point.GetY());
  }

  for (int i{0}; i < centroids_.size(); i++) {
    centroids_[i].SetX(centroid_cache[i].NewX());
    centroids_[i].SetY(centroid_cache[i].NewY());
  }

  if (changed == false) {
    converged_ = true;
  }
}

void Simulator::RunMultiple(const int total) {
  // storage cache
  std::vector<double> costs(total);
  double min_cost{__DBL_MAX__};
  int best_idx{0};
  std::vector<Point> best_points;
  std::vector<Centroid> best_centroids;
  // loop total times
  for (int i{0}; i < total; i++) {
    Reset(false);
    while (!converged_) {
      KMeansIteration();
    }
    if (current_cost_ < min_cost) {
      min_cost = current_cost_;
      best_idx = i;
      // copy the best_points
      best_points = points_;
      best_centroids = centroids_;
    }
    costs[i] = current_cost_;
  }

  // render
  renderer_.SetMultiRunResults(costs, best_points, best_centroids, best_idx);
}

/** @brief returns the squared euclidian distance between two points*/
double Simulator::SquaredEuclidianDistance(const double x1, const double y1,
                                           const double x2,
                                           const double y2) const {
  return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
}

/** @brief sets the point to the correct centroid, returns the distance cost */
double Simulator::ClosestCentroid(Point& point) {
  double min_distance_{__DBL_MAX__};
  int index{1};
  for (int i{0}; i < centroids_.size(); i++) {
    double temp = SquaredEuclidianDistance(
        centroids_[i].GetX(), centroids_[i].GetY(), point.GetX(), point.GetY());
    if (temp < min_distance_) {
      min_distance_ = temp;
      index = i + 1;
    }
  }
  point.SetCentroid(index);
  return min_distance_;
}

double Simulator::GenerateRandomDouble() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dist(kWorldMin, kWorldMax);
  return dist(gen);
}

void Simulator::GenerateCentroids() {
  centroids_.clear();

  for (int i = 0; i < k_; i++) {
    centroids_.push_back(
        Centroid(GenerateRandomDouble(), GenerateRandomDouble()));
  }
}