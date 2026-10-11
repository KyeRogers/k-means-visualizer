#pragma once

#include <string>
#include <vector>

#include "centroid.hpp"
#include "dataset_generator.hpp"
#include "point.hpp"
#include "raylib.h"

class Renderer {
 public:
  enum class CentroidInitialization { Randomized, KMeansPlusPlus, Manual };

  struct InputEvents {
    bool k_selected = false;
    int k = 2;
    CentroidInitialization initialization_method =
        CentroidInitialization::Randomized;
    std::vector<Vector2> manual_centroids;
    bool load_file = false;
    std::string load_filename;
    bool generate_dataset = false;
    int dataset_points = 100;
    DatasetType dataset_type = DatasetType::Random;
    int dataset_blobs = 3;
    int dataset_deviation_x = 30;
    int dataset_deviation_y = 30;
    int dataset_ring_radius = 250;
    bool add_point = false;
    double point_x = 0.0;
    double point_y = 0.0;
    bool step = false;
    bool toggle_running = false;
    int speed_change = 0;
    bool reset = false;
    bool main_menu = false;
    bool hard_reset_requested = false;
    bool multi_run_requested = false;
    int total_runs = 10;
    bool seed_changed = false;
    int seed = 42;
    bool dataset_seed_changed = false;
    int dataset_seed = 1337;
  };

  void Initialize(double world_min, double world_max);
  bool ShouldClose() const;
  InputEvents PollInput(bool converged);
  void Render(const std::vector<Point>& points,
              const std::vector<Centroid>& centroids, double total_cost,
              int iteration, bool converged, double run_interval_seconds);
  void Close();

  void SetSeed(int seed);
  void SetEmptyCentroidNotifications(const std::vector<int>& indices,
                                     int iteration);
  void SetMultiRunResults(const std::vector<double>& costs,
                          const std::vector<Point>& best_points,
                          const std::vector<Centroid>& best_centroids,
                          int best_run);
  void ClearMultiRunResults();
  void BeginHardReset(int k, CentroidInitialization method);
  void ShowMainMenu();

 private:
  enum class Screen {
    Welcome,
    Setup,
    Simulation,
    Manual,
    Summary,
    MultiRun,
    Results
  };

  static constexpr int kMinK = 1;
  static constexpr int kMaxK = 12;
  static constexpr int kMinRuns = 1;
  static constexpr int kMaxRuns = 1000;
  static constexpr int kDefaultRuns = 10;

  Rectangle MapRectangle() const;
  Vector2 WorldToScreen(Vector2 point) const;
  Vector2 ScreenToWorld(Vector2 point) const;
  bool InMap(Vector2 point) const;
  Color ClusterColor(int index) const;
  void DrawButton(Rectangle rectangle, const char* text,
                  bool selected = false) const;
  void DrawMap(const std::vector<Point>& points,
               const std::vector<Centroid>& centroids);
  void DrawSetup() const;
  void DrawWelcome() const;
  void DrawManual(const std::vector<Point>& points);
  void DrawDashboard(const std::vector<Point>& points,
                     const std::vector<Centroid>& centroids, double cost,
                     int iteration, bool converged,
                     double run_interval_seconds);
  void DrawSummary(const std::vector<Point>& points,
                  const std::vector<Centroid>& centroids, double cost,
                  int iteration);
  void DrawResults();
  void DrawHelp() const;
  void DrawTextInput(Rectangle box, const std::string& value,
                     bool active) const;
  void ReadTextInput(std::string& value, bool integer_only, bool& active);
  void ResetView();

  int screen_width_ = 1280;
  int screen_height_ = 800;
  double world_min_ = 0.0;
  double world_max_ = 1000.0;
  float zoom_ = 1.0f;
  Vector2 pan_{0.0f, 0.0f};
  Screen screen_ = Screen::Setup;
  bool help_ = false;
  bool hard_reset_ = false;
  bool summary_dismissed_ = false;
  bool seed_active_ = false;
  bool dataset_seed_active_ = false;
  bool dataset_deviation_x_active_ = false;
  bool dataset_deviation_y_active_ = false;
  bool dataset_ring_radius_active_ = false;
  bool filename_active_ = false;
  std::string seed_input_ = "42";
  std::string dataset_seed_input_ = "1337";
  std::string filename_input_;
  std::string dataset_deviation_x_input_ = "30";
  std::string dataset_deviation_y_input_ = "30";
  std::string dataset_ring_radius_input_ = "250";
  int selected_seed_ = 42;
  int selected_dataset_seed_ = 1337;
  int selected_k_ = 2;
  int selected_runs_ = kDefaultRuns;
  int selected_dataset_points_ = 100;
  DatasetType selected_dataset_type_ = DatasetType::Random;
  int selected_dataset_blobs_ = 3;
  int selected_dataset_deviation_x_ = 30;
  int selected_dataset_deviation_y_ = 30;
  int selected_dataset_ring_radius_ = 250;
  CentroidInitialization selected_method_ =
      CentroidInitialization::Randomized;
  std::vector<Vector2> manual_centroids_;
  std::vector<double> result_costs_;
  std::vector<Point> result_points_;
  std::vector<Centroid> result_centroids_;
  int best_run_ = -1;
  int result_scroll_ = 0;
  std::vector<int> empty_centroids_;
  int empty_iteration_ = -1;
};
