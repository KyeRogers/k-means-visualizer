#include "simulator.hpp"

#include <algorithm>
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
      run_interval_seconds_{2.5},
      run_elapsed_seconds_{0.0},
      k_{0},
      converged_{false},
      centroid_mode_{Renderer::CentroidInitialization::Randomized},
      deferred_kmeans_plus_plus_{false} {};

void Simulator::Reset(const bool complete_reset) {
  if (!complete_reset) {
    for (Point& point : points_) {
      point.SetCentroid(0);
    }
    if (centroid_mode_ == Renderer::CentroidInitialization::Manual) {
      centroids_ = initial_centroids_;
      deferred_kmeans_plus_plus_ = false;
    } else {
      GenerateCentroids();
    }
  } else {
    points_.clear();
    centroids_.clear();
    initial_centroids_.clear();
    deferred_kmeans_plus_plus_ = false;
  }
  iteration_ = 0;
  current_cost_ = 0.0;
  converged_ = false;
  running_ = false;
  run_elapsed_seconds_ = 0.0;
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
      centroid_mode_ = events.initialization_method;
      manual_centroids_ = events.manual_centroids;
      for (Point& point : points_) {
        point.SetCentroid(0);
      }
      centroids_.clear();
      deferred_kmeans_plus_plus_ = false;
      if (centroid_mode_ == Renderer::CentroidInitialization::Manual) {
        for (const Vector2& position : manual_centroids_) {
          centroids_.emplace_back(position.x, position.y);
        }
      } else {
        GenerateCentroids();
      }
      initial_centroids_ = centroids_;
      iteration_ = 0;
      current_cost_ = 0;
      converged_ = false;
      running_ = false;
      run_elapsed_seconds_ = 0.0;
    }

    if (events.load_file) {
      LoadFromFile(events.load_filename);
      iteration_ = 0;
    }

    if (events.multi_run_requested) {
      RunMultiple(events.total_runs);
    }

    if (events.main_menu) {
      renderer_.ShowMainMenu();
      Reset(true);
    }

    if (events.hard_reset_requested) {
      renderer_.BeginHardReset(k_, centroid_mode_);
      running_ = false;
      converged_ = false;
      run_elapsed_seconds_ = 0.0;
    }

    if (events.add_point) {
      points_.push_back(Point(events.point_x, events.point_y, 0));
    }

    if (events.step) {
      KMeansIteration();
    }

    if (events.toggle_running) {
      running_ = !running_;
      run_elapsed_seconds_ = 0.0;
    }

    if (events.speed_change > 0) {
      run_interval_seconds_ = std::max(0.1, run_interval_seconds_ * 0.5);
    } else if (events.speed_change < 0) {
      run_interval_seconds_ = std::min(5.0, run_interval_seconds_ * 2.0);
    }

    if (events.reset) {
      Reset(false);
    }

    // 3. Automatically run K-means if enabled
    if (running_ && !converged_) {
      run_elapsed_seconds_ += GetFrameTime();
      if (run_elapsed_seconds_ >= run_interval_seconds_) {
        KMeansIteration();
        run_elapsed_seconds_ = 0.0;
      }
    } else if (converged_) {
      running_ = false;
    }

    // 4. Render the NEW state
    renderer_.Render(points_, centroids_, current_cost_, iteration_,
                     converged_, run_interval_seconds_);
  }
}
/** @brief returns true if succeded, false if converged  */
void Simulator::KMeansIteration() {
  if (deferred_kmeans_plus_plus_ && !points_.empty()) {
    GenerateCentroids();
  }
  if (points_.empty() || centroids_.empty()) {
    return;
  }

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
    if (centroid_cache[i].count > 0) {
      centroids_[i].SetX(centroid_cache[i].NewX());
      centroids_[i].SetY(centroid_cache[i].NewY());
    }
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
  if (centroid_mode_ == Renderer::CentroidInitialization::Manual) {
    return;
  }

  centroids_.clear();
  deferred_kmeans_plus_plus_ = false;

  if (centroid_mode_ == Renderer::CentroidInitialization::Randomized) {
    for (int i = 0; i < k_; i++) {
      centroids_.emplace_back(GenerateRandomDouble(), GenerateRandomDouble());
    }
    return;
  }

  if (points_.empty()) {
    deferred_kmeans_plus_plus_ = true;
    for (int i = 0; i < k_; i++) {
      centroids_.emplace_back(GenerateRandomDouble(), GenerateRandomDouble());
    }
    return;
  }

  std::mt19937 generator(std::random_device{}());
  std::uniform_int_distribution<std::size_t> first_point(0,
                                                         points_.size() - 1);
  const std::size_t first_index = first_point(generator);
  const Point& first = points_[first_index];
  centroids_.emplace_back(first.GetX(), first.GetY());
  std::vector<bool> selected_points(points_.size(), false);
  selected_points[first_index] = true;

  while (centroids_.size() < static_cast<std::size_t>(k_)) {
    std::vector<double> weights(points_.size(), 0.0);
    double total_weight = 0.0;

    for (std::size_t i = 0; i < points_.size(); ++i) {
      double nearest_distance = __DBL_MAX__;
      for (const Centroid& centroid : centroids_) {
        nearest_distance =
            std::min(nearest_distance,
                     SquaredEuclidianDistance(
                         points_[i].GetX(), points_[i].GetY(),
                         centroid.GetX(), centroid.GetY()));
      }
      weights[i] = nearest_distance;
      total_weight += nearest_distance;
    }

    std::size_t selected_point = 0;
    if (total_weight > 0.0) {
      std::discrete_distribution<std::size_t> choose_point(weights.begin(),
                                                           weights.end());
      selected_point = choose_point(generator);
    } else {
      std::vector<std::size_t> unselected_points;
      for (std::size_t i = 0; i < selected_points.size(); ++i) {
        if (!selected_points[i]) {
          unselected_points.push_back(i);
        }
      }

      if (unselected_points.empty()) {
        std::uniform_int_distribution<std::size_t> choose_point(
            0, points_.size() - 1);
        selected_point = choose_point(generator);
      } else {
        std::uniform_int_distribution<std::size_t> choose_point(
            0, unselected_points.size() - 1);
        selected_point = unselected_points[choose_point(generator)];
      }
    }

    selected_points[selected_point] = true;
    centroids_.emplace_back(points_[selected_point].GetX(),
                            points_[selected_point].GetY());
  }
}