#pragma once

#include <string>
#include <vector>

#include "centroid.hpp"
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
  };

  Renderer() = default;
  ~Renderer() = default;

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
  struct NamedColor {
    const char* name;
    Color color;
  };
  static constexpr int kColorCount = 13;
  static const NamedColor kPalette[kColorCount];
  static constexpr int kMinK = 1;
  static constexpr int kMaxK = 12;
  static constexpr int kMinRuns = 1;
  static constexpr int kMaxRuns = 1000;
  static constexpr int kDefaultRuns = 10;
  static constexpr double kDefaultWorldMin = -100.0;
  static constexpr double kDefaultWorldMax = 100.0;
  static constexpr int kTopBarHeight = 55;
  static constexpr int kBottomBarHeight = 58;
  static constexpr int kLeftPanelWidth = 180;
  static constexpr float kPointRadius = 4.0f;
  static constexpr float kCentroidSize = 10.0f;

  void DrawTopBar(int k, int iteration, double total_cost, int seed) const;
  void DrawBottomBar(double run_interval_seconds) const;
  void DrawSidePanel(int k) const;
  void DrawPoints(const std::vector<Point>& points, Rectangle clip) const;
  void DrawCentroids(const std::vector<Centroid>& centroids,
                     Rectangle clip) const;
  Color GetPointColor(int centroid) const;
  Color GetCentroidColor(int centroid_index) const;
  Vector2 WorldToScreen(Vector2 point) const;
  Vector2 WorldToScreen(Vector2 point, Rectangle plot) const;
  Vector2 ScreenToWorld(Vector2 point) const;
  Vector2 ScreenToWorld(Vector2 point, Rectangle plot) const;
  bool IsPointInsidePlot(Vector2 point) const;
  bool IsPointInsidePlot(Vector2 point, Rectangle plot) const;
  void DrawButton(Rectangle rectangle, const char* text,
                  bool selected = false) const;
  void DrawSetup() const;
  void DrawManualSetup(const std::vector<Point>& points) const;
  void DrawMultiRunSetup() const;
  void DrawMultiRunResults() const;
  void DrawConvergedScreen(const std::vector<Point>& points,
                           const std::vector<Centroid>& centroids,
                           double total_cost, int iteration) const;
  void DrawConvergedMap(const std::vector<Point>& points,
                        const std::vector<Centroid>& centroids,
                        double total_cost, int iteration) const;
  void DrawHelp() const;
  void DrawEmptyCentroidNotice() const;
  void FitView(const std::vector<Point>& points,
               const std::vector<Centroid>& centroids);

  int screen_width_ = 1280;
  int screen_height_ = 800;
  double world_min_ = kDefaultWorldMin;
  double world_max_ = kDefaultWorldMax;
  float zoom_ = 1.0f;
  Vector2 pan_ = {0.0f, 0.0f};
  bool show_help_ = false;
  bool show_setup_ = true;
  bool show_manual_setup_ = false;
  bool show_multi_run_setup_ = false;
  bool show_multi_run_results_ = false;
  bool show_converged_map_ = false;
  bool hard_reset_setup_ = false;
  bool filename_active_ = false;
  std::string filename_input_;
  int selected_k_ = 2;
  int selected_runs_ = kDefaultRuns;
  CentroidInitialization selected_method_ = CentroidInitialization::Randomized;
  std::vector<Vector2> manual_centroids_;
  std::vector<double> multi_run_costs_;
  std::vector<Point> multi_run_points_;
  std::vector<Centroid> multi_run_centroids_;
  int multi_run_best_run_ = -1;
  int multi_run_cost_scroll_ = 0;
  int selected_seed_ = 42;
  bool seed_input_active_ = false;
  std::string seed_input_ = "42";
  std::vector<int> empty_centroid_indices_;
  int empty_centroid_iteration_ = -1;
};