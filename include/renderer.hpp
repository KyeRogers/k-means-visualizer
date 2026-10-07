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
  };

  /**
   * Initializes the renderer and sets the simulation coordinate bounds.
   *
   * @param world_min Minimum world coordinate.
   * @param world_max Maximum world coordinate.
   */
  void Initialize(double world_min, double world_max);

  /**
   * Checks whether the renderer should close.
   *
   * @return True if the window should close.
   */
  bool ShouldClose() const;

  /**
   * Polls keyboard and mouse input.
   *
   * @param converged Whether the simulation has converged.
   *
   * @return Input events generated during this frame.
   */
  InputEvents PollInput(bool converged);

  /**
   * Renders the current simulation state.
   *
   * @param points Points belonging to the simulation.
   * @param centroids Centroids belonging to the simulation.
   * @param total_cost Current total cost.
   * @param iteration Current iteration number.
   * @param converged Whether the simulation has converged.
   */
  void Render(const std::vector<Point>& points,
              const std::vector<Centroid>& centroids, double total_cost,
              int iteration, bool converged);

  /**
   * Closes the renderer.
   */
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

  static constexpr float kPointRadius = 5.0f;
  static constexpr float kCentroidSize = 11.0f;

  static constexpr double kDefaultWorldMin = 0.0;
  static constexpr double kDefaultWorldMax = 1000.0;

  void DrawTopBar(int k, int iteration, double total_cost) const;
  void DrawBottomBar() const;
  void DrawSidePanel(int k) const;
  void DrawHelp() const;
  void DrawSetup() const;

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