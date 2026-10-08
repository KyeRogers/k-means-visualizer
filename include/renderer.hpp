#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <string>
#include <vector>

#include "centroid.hpp"
#include "point.hpp"
#include "raylib.h"

class Renderer {
 public:
  /**
   * Contains user input events reported to the simulator.
   */
  struct InputEvents {
    bool k_selected = false;
    int k = 2;

    bool load_file = false;
    std::string load_filename;

    bool add_point = false;
    double point_x = 0.0;
    double point_y = 0.0;

    bool step = false;
    bool toggle_running = false;

    bool reset = false;
    bool main_menu = false;

    // Multi-run mode.
    bool multi_run_requested = false;
    int total_runs = 10;
  };

  /**
   * Initializes the renderer and sets the simulation coordinate bounds.
   *
   * @param world_min Minimum world coordinate.
   * @param world_max Maximum world coordinate.
   */
  void Initialize(double world_min, double world_max);

  bool ShouldClose() const;

  InputEvents PollInput(bool converged);

  void Render(const std::vector<Point>& points,
              const std::vector<Centroid>& centroids, double total_cost,
              int iteration, bool converged);

  /**
   * Supplies the result of a multi-run experiment.
   *
   * The renderer copies the costs and the best solution so it can display
   * the complete cost vector and the final/best clustering.
   *
   * @param costs Cost produced by each run.
   * @param best_points Points belonging to the best run.
   * @param best_centroids Centroids belonging to the best run.
   * @param best_run Zero-based index of the best run.
   */
  void SetMultiRunResults(const std::vector<double>& costs,
                          const std::vector<Point>& best_points,
                          const std::vector<Centroid>& best_centroids,
                          int best_run);

  /**
   * Returns the renderer to the normal simulation state and clears the
   * previous multi-run result.
   */
  void ClearMultiRunResults();

  void Close();

 private:
  struct NamedColor {
    const char* name;
    Color color;
  };

  static constexpr int kTopBarHeight = 60;
  static constexpr int kBottomBarHeight = 58;
  static constexpr int kLeftPanelWidth = 180;

  static constexpr int kColorCount = 13;

  static constexpr int kMinK = 1;
  static constexpr int kMaxK = kColorCount - 1;

  static constexpr int kMinRuns = 1;
  static constexpr int kMaxRuns = 1000;
  static constexpr int kDefaultRuns = 10;

  static constexpr float kPointRadius = 5.0f;
  static constexpr float kCentroidSize = 11.0f;

  static constexpr double kDefaultWorldMin = 0.0;
  static constexpr double kDefaultWorldMax = 1000.0;

  void DrawTopBar(int k, int iteration, double total_cost) const;
  void DrawBottomBar() const;
  void DrawSidePanel(int k) const;
  void DrawHelp() const;
  void DrawSetup() const;

  void DrawMultiRunSetup() const;
  void DrawMultiRunResults() const;

  void DrawConvergedScreen(const std::vector<Point>& points,
                           const std::vector<Centroid>& centroids,
                           double total_cost, int iteration) const;

  void DrawButton(Rectangle rectangle, const char* text,
                  bool selected = false) const;

  void DrawPoints(const std::vector<Point>& points) const;
  void DrawCentroids(const std::vector<Centroid>& centroids) const;

  Color GetPointColor(int centroid) const;
  Color GetCentroidColor(int centroid_index) const;

  Vector2 WorldToScreen(Vector2 point) const;
  Vector2 ScreenToWorld(Vector2 point) const;

  bool IsPointInsidePlot(Vector2 point) const;

  void FitView(const std::vector<Point>& points,
               const std::vector<Centroid>& centroids);

  std::string filename_input_;
  bool filename_active_ = false;
  bool show_help_ = false;
  bool show_setup_ = true;

  bool show_multi_run_setup_ = false;
  bool show_multi_run_results_ = false;
  int selected_runs_ = kDefaultRuns;

  std::vector<double> multi_run_costs_;
  std::vector<Point> multi_run_points_;
  std::vector<Centroid> multi_run_centroids_;
  int multi_run_best_run_ = -1;
  int multi_run_cost_scroll_ = 0;

  int selected_k_ = 2;

  double world_min_ = kDefaultWorldMin;
  double world_max_ = kDefaultWorldMax;

  float zoom_ = 1.0f;
  Vector2 pan_ = {0.0f, 0.0f};

  int screen_width_ = 1280;
  int screen_height_ = 720;

  static const NamedColor kPalette[kColorCount];
};

#endif
