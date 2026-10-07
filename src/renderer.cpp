#include "renderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

namespace {

constexpr float kPanelPadding = 15.0f;
constexpr float kButtonHeight = 34.0f;
constexpr float kButtonSpacing = 8.0f;

const Color kBackground = {245, 245, 245, 255};
const Color kPanel = {230, 230, 230, 255};
const Color kPanelDark = {210, 210, 210, 255};
const Color kBorder = {180, 180, 180, 255};
const Color kText = {35, 35, 35, 255};
const Color kMutedText = {100, 100, 100, 255};
const Color kWhite = {255, 255, 255, 255};
const Color kUnassigned = {130, 130, 130, 255};

float ClampFloat(float value, float minimum, float maximum) {
  return std::max(minimum, std::min(value, maximum));
}

}  // namespace

const Renderer::NamedColor Renderer::kPalette[Renderer::kColorCount] = {
    {"Unassigned", {130, 130, 130, 255}},
    {"Red", {220, 70, 70, 255}},
    {"Blue", {70, 120, 220, 255}},
    {"Green", {70, 170, 90, 255}},
    {"Orange", {230, 145, 55, 255}},
    {"Purple", {150, 85, 190, 255}},
    {"Cyan", {50, 170, 180, 255}},
    {"Pink", {220, 90, 150, 255}},
    {"Yellow", {220, 190, 55, 255}},
    {"Brown", {155, 105, 70, 255}},
    {"Lime", {125, 180, 65, 255}},
    {"Magenta", {190, 70, 170, 255}},
    {"Teal", {55, 145, 125, 255}},
};

void Renderer::Initialize(double world_min, double world_max) {
  if (world_min >= world_max) {
    world_min = kDefaultWorldMin;
    world_max = kDefaultWorldMax;
  }

  world_min_ = world_min;
  world_max_ = world_max;

  InitWindow(screen_width_, screen_height_, "K-Means Visualizer");
  SetTargetFPS(60);

  filename_input_.clear();
  filename_active_ = false;
  show_help_ = false;
  show_setup_ = true;

  selected_k_ = 2;

  zoom_ = 1.0f;
  pan_ = {0.0f, 0.0f};
}

bool Renderer::ShouldClose() const {
  return WindowShouldClose();
}

Renderer::InputEvents Renderer::PollInput(bool converged) {
  InputEvents events;

  screen_width_ = GetScreenWidth();
  screen_height_ = GetScreenHeight();

  if (IsKeyPressed(KEY_F1) || IsKeyPressed(KEY_H)) {
    show_help_ = !show_help_;
  }

  if (show_help_) {
    int character = GetCharPressed();

    while (character > 0) {
      if (character >= 32 && character <= 126) {
        filename_input_ += static_cast<char>(character);
      }

      character = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && !filename_input_.empty()) {
      filename_input_.pop_back();
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
      filename_active_ = false;
      show_help_ = false;
    }

    if (IsKeyPressed(KEY_ENTER) && !filename_input_.empty()) {
      events.load_file = true;
      events.load_filename = filename_input_;
      filename_input_.clear();
      filename_active_ = false;
      show_help_ = false;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 mouse = GetMousePosition();

      Rectangle filename_box = {
          25.0f,
          315.0f,
          static_cast<float>(screen_width_) - 50.0f,
          38.0f};

      if (CheckCollisionPointRec(mouse, filename_box)) {
        filename_active_ = true;
      }
    }

    return events;
  }

  // ------------------------------------------------------------
  // CONVERGED SCREEN
  // ------------------------------------------------------------
  //
  // Once the simulation has converged, don't allow normal
  // simulation controls to fire. Only allow Reset / Main Menu.
  //
  if (converged) {
    if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_ENTER)) {
      events.reset = true;
    }

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_M)) {
      events.main_menu = true;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 mouse = GetMousePosition();

      Rectangle reset_button = {
          screen_width_ / 2.0f - 190.0f,
          screen_height_ / 2.0f + 90.0f,
          160.0f,
          45.0f};

      Rectangle menu_button = {
          screen_width_ / 2.0f + 30.0f,
          screen_height_ / 2.0f + 90.0f,
          160.0f,
          45.0f};

      if (CheckCollisionPointRec(mouse, reset_button)) {
        events.reset = true;
      } else if (CheckCollisionPointRec(mouse, menu_button)) {
        events.main_menu = true;
      }
    }

    return events;
  }

  // ------------------------------------------------------------
  // SETUP SCREEN
  // ------------------------------------------------------------

  if (show_setup_) {
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
      selected_k_ = std::max(kMinK, selected_k_ - 1);
    }

    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
      selected_k_ = std::min(kMaxK, selected_k_ + 1);
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
      events.k_selected = true;
      events.k = selected_k_;
      show_setup_ = false;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 mouse = GetMousePosition();

      Rectangle minus_button = {
          screen_width_ / 2.0f - 90.0f,
          screen_height_ / 2.0f - 20.0f,
          50.0f,
          40.0f};

      Rectangle plus_button = {
          screen_width_ / 2.0f + 40.0f,
          screen_height_ / 2.0f - 20.0f,
          50.0f,
          40.0f};

      Rectangle start_button = {
          screen_width_ / 2.0f - 90.0f,
          screen_height_ / 2.0f + 50.0f,
          180.0f,
          42.0f};

      if (CheckCollisionPointRec(mouse, minus_button)) {
        selected_k_ = std::max(kMinK, selected_k_ - 1);
      } else if (CheckCollisionPointRec(mouse, plus_button)) {
        selected_k_ = std::min(kMaxK, selected_k_ + 1);
      } else if (CheckCollisionPointRec(mouse, start_button)) {
        events.k_selected = true;
        events.k = selected_k_;
        show_setup_ = false;
      }
    }

    return events;
  }

  // ------------------------------------------------------------
  // NORMAL SIMULATION INPUT
  // ------------------------------------------------------------

  if (IsKeyPressed(KEY_SPACE)) {
    events.step = true;
  }

  if (IsKeyPressed(KEY_R)) {
    events.toggle_running = true;
  }

  if (IsKeyPressed(KEY_ESCAPE)) {
    events.reset = true;
  }

  if (IsKeyPressed(KEY_F2)) {
    zoom_ = 1.0f;
    pan_ = {0.0f, 0.0f};
  }

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    Vector2 mouse = GetMousePosition();

    Rectangle step_button = {
        15.0f,
        static_cast<float>(screen_height_) - kBottomBarHeight + 12.0f,
        90.0f,
        34.0f};

    Rectangle run_button = {
        115.0f,
        static_cast<float>(screen_height_) - kBottomBarHeight + 12.0f,
        100.0f,
        34.0f};

    Rectangle reset_button = {
        225.0f,
        static_cast<float>(screen_height_) - kBottomBarHeight + 12.0f,
        90.0f,
        34.0f};

    if (CheckCollisionPointRec(mouse, step_button)) {
      events.step = true;
    } else if (CheckCollisionPointRec(mouse, run_button)) {
      events.toggle_running = true;
    } else if (CheckCollisionPointRec(mouse, reset_button)) {
      events.reset = true;
    } else if (IsPointInsidePlot(mouse)) {
      Vector2 world = ScreenToWorld(mouse);

      events.add_point = true;
      events.point_x = world.x;
      events.point_y = world.y;
    }
  }

  if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
    Vector2 delta = GetMouseDelta();

    pan_.x += delta.x;
    pan_.y += delta.y;
  }

  float wheel = GetMouseWheelMove();

  if (wheel != 0.0f) {
    Vector2 mouse = GetMousePosition();
    Vector2 before_zoom = ScreenToWorld(mouse);

    if (wheel > 0.0f) {
      zoom_ *= 1.1f;
    } else {
      zoom_ /= 1.1f;
    }

    zoom_ = ClampFloat(zoom_, 0.1f, 20.0f);

    Vector2 after_zoom = WorldToScreen(before_zoom);

    pan_.x += mouse.x - after_zoom.x;
    pan_.y += mouse.y - after_zoom.y;
  }

  return events;
}

void Renderer::Render(const std::vector<Point>& points,
                      const std::vector<Centroid>& centroids,
                      double total_cost,
                      int iteration,
                      bool converged) {
  screen_width_ = GetScreenWidth();
  screen_height_ = GetScreenHeight();

  BeginDrawing();
  ClearBackground(kBackground);

  // ------------------------------------------------------------
  // CONVERGED SCREEN
  // ------------------------------------------------------------

  if (converged) {
    DrawConvergedScreen(
        points,
        centroids,
        total_cost,
        iteration);

    EndDrawing();
    return;
  }

  // ------------------------------------------------------------
  // SETUP SCREEN
  // ------------------------------------------------------------

  if (show_setup_) {
    DrawSetup();

    EndDrawing();
    return;
  }

  // ------------------------------------------------------------
  // NORMAL SIMULATION SCREEN
  // ------------------------------------------------------------

  DrawTopBar(
      static_cast<int>(centroids.size()),
      iteration,
      total_cost);

  DrawRectangle(
      0,
      kTopBarHeight,
      kLeftPanelWidth,
      screen_height_ - kTopBarHeight - kBottomBarHeight,
      kPanel);

  DrawRectangle(
      kLeftPanelWidth,
      kTopBarHeight,
      screen_width_ - kLeftPanelWidth,
      screen_height_ - kTopBarHeight - kBottomBarHeight,
      kWhite);

  DrawSidePanel(static_cast<int>(centroids.size()));

  DrawPoints(points);
  DrawCentroids(centroids);

  DrawBottomBar();

  if (show_help_) {
    DrawHelp();
  }

  EndDrawing();
}

void Renderer::Close() {
  if (IsWindowReady()) {
    CloseWindow();
  }
}

void Renderer::DrawTopBar(int k,
                          int iteration,
                          double total_cost) const {
  DrawRectangle(
      0,
      0,
      screen_width_,
      kTopBarHeight,
      kPanelDark);

  DrawText(
      "K-MEANS",
      18,
      17,
      24,
      kText);

  char buffer[128];

  std::snprintf(
      buffer,
      sizeof(buffer),
      "K: %d",
      k);

  DrawText(
      buffer,
      220,
      20,
      20,
      kText);

  std::snprintf(
      buffer,
      sizeof(buffer),
      "Iteration: %d",
      iteration);

  DrawText(
      buffer,
      290,
      20,
      20,
      kText);

  std::snprintf(
      buffer,
      sizeof(buffer),
      "Cost: %.4f",
      total_cost);

  DrawText(
      buffer,
      470,
      20,
      20,
      kText);

  DrawText(
      "F1: Help",
      screen_width_ - 100,
      21,
      16,
      kMutedText);
}

void Renderer::DrawBottomBar() const {
  int y = screen_height_ - kBottomBarHeight;

  DrawRectangle(
      0,
      y,
      screen_width_,
      kBottomBarHeight,
      kPanelDark);

  DrawButton(
      {15.0f,
       static_cast<float>(y) + 12.0f,
       90.0f,
       kButtonHeight},
      "STEP");

  DrawButton(
      {115.0f,
       static_cast<float>(y) + 12.0f,
       100.0f,
       kButtonHeight},
      "RUN / STOP");

  DrawButton(
      {225.0f,
       static_cast<float>(y) + 12.0f,
       90.0f,
       kButtonHeight},
      "RESET");

  DrawText(
      "SPACE: Step",
      345,
      y + 20,
      16,
      kMutedText);

  DrawText(
      "R: Run/Stop",
      455,
      y + 20,
      16,
      kMutedText);

  DrawText(
      "ESC: Reset",
      570,
      y + 20,
      16,
      kMutedText);

  DrawText(
      "F2: Reset View",
      675,
      y + 20,
      16,
      kMutedText);
}

void Renderer::DrawSidePanel(int k) const {
  DrawText(
      "CLUSTERS",
      18,
      kTopBarHeight + 18,
      18,
      kText);

  int y = kTopBarHeight + 55;

  for (int i = 0; i < k; ++i) {
    int color_index = i + 1;

    if (color_index >= kColorCount) {
      color_index = 1 + (i % (kColorCount - 1));
    }

    DrawCircle(
        28,
        y + 9,
        7.0f,
        kPalette[color_index].color);

    char label[64];

    std::snprintf(
        label,
        sizeof(label),
        "Centroid %d",
        i);

    DrawText(
        label,
        45,
        y,
        16,
        kText);

    y += 28;
  }

  DrawLine(
      15,
      y + 5,
      kLeftPanelWidth - 15,
      y + 5,
      kBorder);

  DrawText(
      "WORLD",
      18,
      y + 25,
      18,
      kText);

  char buffer[128];

  std::snprintf(
      buffer,
      sizeof(buffer),
      "X: %.1f - %.1f",
      world_min_,
      world_max_);

  DrawText(
      buffer,
      18,
      y + 52,
      14,
      kMutedText);

  std::snprintf(
      buffer,
      sizeof(buffer),
      "Y: %.1f - %.1f",
      world_min_,
      world_max_);

  DrawText(
      buffer,
      18,
      y + 73,
      14,
      kMutedText);

  std::snprintf(
      buffer,
      sizeof(buffer),
      "Zoom: %.2fx",
      zoom_);

  DrawText(
      buffer,
      18,
      y + 105,
      14,
      kMutedText);

  DrawText(
      "Middle mouse: Pan",
      18,
      y + 130,
      13,
      kMutedText);

  DrawText(
      "Wheel: Zoom",
      18,
      y + 149,
      13,
      kMutedText);
}

void Renderer::DrawPoints(
    const std::vector<Point>& points) const {
  for (const Point& point : points) {
    Vector2 screen_point = WorldToScreen(
        {static_cast<float>(point.GetX()),
         static_cast<float>(point.GetY())});

    if (screen_point.x < kLeftPanelWidth ||
        screen_point.x > screen_width_ ||
        screen_point.y < kTopBarHeight ||
        screen_point.y >
            screen_height_ - kBottomBarHeight) {
      continue;
    }

    DrawCircleV(
        screen_point,
        kPointRadius,
        GetPointColor(point.GetCentroid()));
  }
}

void Renderer::DrawCentroids(
    const std::vector<Centroid>& centroids) const {
  for (std::size_t i = 0; i < centroids.size(); ++i) {
    Vector2 screen_point = WorldToScreen(
        {static_cast<float>(centroids[i].GetX()),
         static_cast<float>(centroids[i].GetY())});

    Color color = GetCentroidColor(
        static_cast<int>(i));

    DrawLineEx(
        {screen_point.x - kCentroidSize,
         screen_point.y - kCentroidSize},
        {screen_point.x + kCentroidSize,
         screen_point.y + kCentroidSize},
        4.0f,
        color);

    DrawLineEx(
        {screen_point.x - kCentroidSize,
         screen_point.y + kCentroidSize},
        {screen_point.x + kCentroidSize,
         screen_point.y - kCentroidSize},
        4.0f,
        color);
  }
}

Color Renderer::GetPointColor(int centroid) const {
  if (centroid <= 0) {
    return kUnassigned;
  }

  int color_index = centroid;

  if (color_index >= kColorCount) {
    color_index =
        1 + ((color_index - 1) % (kColorCount - 1));
  }

  return kPalette[color_index].color;
}

Color Renderer::GetCentroidColor(
    int centroid_index) const {
  int color_index = centroid_index + 1;

  if (color_index >= kColorCount) {
    color_index =
        1 + (centroid_index % (kColorCount - 1));
  }

  return kPalette[color_index].color;
}

Vector2 Renderer::WorldToScreen(Vector2 point) const {
  float plot_left =
      static_cast<float>(kLeftPanelWidth);

  float plot_top =
      static_cast<float>(kTopBarHeight);

  float plot_width =
      static_cast<float>(
          screen_width_ - kLeftPanelWidth);

  float plot_height =
      static_cast<float>(
          screen_height_ -
          kTopBarHeight -
          kBottomBarHeight);

  double world_range =
      world_max_ - world_min_;

  if (world_range <= 0.0) {
    return {
        plot_left + plot_width / 2.0f,
        plot_top + plot_height / 2.0f};
  }

  float normalized_x =
      static_cast<float>(
          (static_cast<double>(point.x) -
           world_min_) /
          world_range);

  float normalized_y =
      static_cast<float>(
          (static_cast<double>(point.y) -
           world_min_) /
          world_range);

  float x =
      plot_left +
      normalized_x * plot_width;

  float y =
      plot_top +
      (1.0f - normalized_y) * plot_height;

  x =
      plot_left +
      (x - plot_left) * zoom_ +
      pan_.x;

  y =
      plot_top +
      (y - plot_top) * zoom_ +
      pan_.y;

  return {x, y};
}

Vector2 Renderer::ScreenToWorld(Vector2 point) const {
  float plot_left =
      static_cast<float>(kLeftPanelWidth);

  float plot_top =
      static_cast<float>(kTopBarHeight);

  float plot_width =
      static_cast<float>(
          screen_width_ - kLeftPanelWidth);

  float plot_height =
      static_cast<float>(
          screen_height_ -
          kTopBarHeight -
          kBottomBarHeight);

  double world_range =
      world_max_ - world_min_;

  if (world_range <= 0.0 ||
      plot_width <= 0.0f ||
      plot_height <= 0.0f) {
    return {
        static_cast<float>(world_min_),
        static_cast<float>(world_min_)};
  }

  float unpanned_x =
      plot_left +
      (point.x - plot_left - pan_.x) /
          zoom_;

  float unpanned_y =
      plot_top +
      (point.y - plot_top - pan_.y) /
          zoom_;

  float normalized_x =
      (unpanned_x - plot_left) /
      plot_width;

  float normalized_y =
      1.0f -
      (unpanned_y - plot_top) /
          plot_height;

  double world_x =
      world_min_ +
      static_cast<double>(normalized_x) *
          world_range;

  double world_y =
      world_min_ +
      static_cast<double>(normalized_y) *
          world_range;

  return {
      static_cast<float>(world_x),
      static_cast<float>(world_y)};
}

bool Renderer::IsPointInsidePlot(Vector2 point) const {
  return point.x >= kLeftPanelWidth &&
         point.x <= screen_width_ &&
         point.y >= kTopBarHeight &&
         point.y <=
             screen_height_ -
                 kBottomBarHeight;
}

void Renderer::DrawButton(
    Rectangle rectangle,
    const char* text,
    bool selected) const {
  Color background =
      selected ? kPanelDark : kWhite;

  DrawRectangleRec(
      rectangle,
      background);

  DrawRectangleLinesEx(
      rectangle,
      1.0f,
      kBorder);

  int text_width =
      MeasureText(text, 16);

  DrawText(
      text,
      static_cast<int>(
          rectangle.x +
          (rectangle.width -
           text_width) /
              2.0f),
      static_cast<int>(
          rectangle.y +
          (rectangle.height -
           16) /
              2.0f),
      16,
      kText);
}

void Renderer::DrawSetup() const {
  DrawRectangle(
      0,
      0,
      screen_width_,
      screen_height_,
      kBackground);

  const char* title = "K-MEANS";

  int title_width =
      MeasureText(title, 42);

  DrawText(
      title,
      (screen_width_ - title_width) / 2,
      screen_height_ / 2 - 150,
      42,
      kText);

  const char* subtitle =
      "Select number of clusters";

  int subtitle_width =
      MeasureText(subtitle, 20);

  DrawText(
      subtitle,
      (screen_width_ - subtitle_width) / 2,
      screen_height_ / 2 - 90,
      20,
      kMutedText);

  Rectangle minus_button = {
      screen_width_ / 2.0f - 90.0f,
      screen_height_ / 2.0f - 20.0f,
      50.0f,
      40.0f};

  Rectangle plus_button = {
      screen_width_ / 2.0f + 40.0f,
      screen_height_ / 2.0f - 20.0f,
      50.0f,
      40.0f};

  DrawButton(
      minus_button,
      "-");

  DrawButton(
      plus_button,
      "+");

  char k_text[32];

  std::snprintf(
      k_text,
      sizeof(k_text),
      "%d",
      selected_k_);

  int k_width =
      MeasureText(k_text, 32);

  DrawText(
      k_text,
      (screen_width_ - k_width) / 2,
      screen_height_ / 2 - 17,
      32,
      kText);

  Rectangle start_button = {
      screen_width_ / 2.0f - 90.0f,
      screen_height_ / 2.0f + 50.0f,
      180.0f,
      42.0f};

  DrawButton(
      start_button,
      "START");

  const char* hint =
      "Use +/- or Left/Right arrows, then Enter";

  int hint_width =
      MeasureText(hint, 16);

  DrawText(
      hint,
      (screen_width_ - hint_width) / 2,
      screen_height_ / 2 + 115,
      16,
      kMutedText);
}

void Renderer::DrawConvergedScreen(
    const std::vector<Point>& points,
    const std::vector<Centroid>& centroids,
    double total_cost,
    int iteration) const {

  DrawRectangle(
      0,
      0,
      screen_width_,
      screen_height_,
      kBackground);

  // ------------------------------------------------------------
  // TITLE
  // ------------------------------------------------------------

  const char* title =
      "K-MEANS CONVERGED";

  int title_size = 40;

  int title_width =
      MeasureText(title, title_size);

  DrawText(
      title,
      (screen_width_ - title_width) / 2,
      70,
      title_size,
      kText);

  const char* subtitle =
      "The cluster assignments are no longer changing.";

  int subtitle_size = 18;

  int subtitle_width =
      MeasureText(subtitle, subtitle_size);

  DrawText(
      subtitle,
      (screen_width_ - subtitle_width) / 2,
      125,
      subtitle_size,
      kMutedText);

  // ------------------------------------------------------------
  // SUMMARY PANEL
  // ------------------------------------------------------------

  float panel_width = 500.0f;
  float panel_height = 250.0f;

  float panel_x =
      (screen_width_ - panel_width) / 2.0f;

  float panel_y = 175.0f;

  Rectangle panel = {
      panel_x,
      panel_y,
      panel_width,
      panel_height};

  DrawRectangleRec(
      panel,
      kWhite);

  DrawRectangleLinesEx(
      panel,
      2.0f,
      kBorder);

  DrawText(
      "SUMMARY",
      static_cast<int>(panel_x + 25),
      static_cast<int>(panel_y + 20),
      22,
      kText);

  char buffer[128];

  // Points
  std::snprintf(
      buffer,
      sizeof(buffer),
      "Points: %d",
      static_cast<int>(points.size()));

  DrawText(
      buffer,
      static_cast<int>(panel_x + 25),
      static_cast<int>(panel_y + 65),
      18,
      kMutedText);

  // Clusters
  std::snprintf(
      buffer,
      sizeof(buffer),
      "Clusters: %d",
      static_cast<int>(centroids.size()));

  DrawText(
      buffer,
      static_cast<int>(panel_x + 25),
      static_cast<int>(panel_y + 100),
      18,
      kMutedText);

  // Iterations
  std::snprintf(
      buffer,
      sizeof(buffer),
      "Iterations: %d",
      iteration);

  DrawText(
      buffer,
      static_cast<int>(panel_x + 25),
      static_cast<int>(panel_y + 135),
      18,
      kMutedText);

  // Final cost
  std::snprintf(
      buffer,
      sizeof(buffer),
      "Final cost: %.4f",
      total_cost);

  DrawText(
      buffer,
      static_cast<int>(panel_x + 25),
      static_cast<int>(panel_y + 170),
      18,
      kMutedText);

  // ------------------------------------------------------------
  // BUTTONS
  // ------------------------------------------------------------

  Rectangle reset_button = {
      screen_width_ / 2.0f - 190.0f,
      screen_height_ / 2.0f + 90.0f,
      160.0f,
      45.0f};

  Rectangle menu_button = {
      screen_width_ / 2.0f + 30.0f,
      screen_height_ / 2.0f + 90.0f,
      160.0f,
      45.0f};

  DrawButton(
      reset_button,
      "RESET");

  DrawButton(
      menu_button,
      "MAIN MENU");

  // ------------------------------------------------------------
  // KEYBOARD HINTS
  // ------------------------------------------------------------

  const char* hints =
      "R / Enter: Reset     M / Esc: Main Menu";

  int hints_width =
      MeasureText(hints, 16);

  DrawText(
      hints,
      (screen_width_ - hints_width) / 2,
      screen_height_ - 55,
      16,
      kMutedText);
}

void Renderer::DrawHelp() const {
  DrawRectangle(
      0,
      0,
      screen_width_,
      screen_height_,
      Fade(BLACK, 0.55f));

  float panel_width =
      std::min(
          650.0f,
          static_cast<float>(screen_width_) - 40.0f);

  float panel_height =
      std::min(
          500.0f,
          static_cast<float>(screen_height_) - 40.0f);

  float panel_x =
      (screen_width_ - panel_width) / 2.0f;

  float panel_y =
      (screen_height_ - panel_height) / 2.0f;

  Rectangle panel = {
      panel_x,
      panel_y,
      panel_width,
      panel_height};

  DrawRectangleRec(
      panel,
      kWhite);

  DrawRectangleLinesEx(
      panel,
      2.0f,
      kBorder);

  DrawText(
      "HELP",
      static_cast<int>(panel_x + 20),
      static_cast<int>(panel_y + 18),
      28,
      kText);

  int y =
      static_cast<int>(panel_y + 65);

  DrawText(
      "SPACE",
      static_cast<int>(panel_x + 25),
      y,
      16,
      kText);

  DrawText(
      "Step",
      static_cast<int>(panel_x + 130),
      y,
      16,
      kMutedText);

  y += 27;

  DrawText(
      "R",
      static_cast<int>(panel_x + 25),
      y,
      16,
      kText);

  DrawText(
      "Start / stop event",
      static_cast<int>(panel_x + 130),
      y,
      16,
      kMutedText);

  y += 27;

  DrawText(
      "ESC",
      static_cast<int>(panel_x + 25),
      y,
      16,
      kText);

  DrawText(
      "Reset event",
      static_cast<int>(panel_x + 130),
      y,
      16,
      kMutedText);

  y += 27;

  DrawText(
      "F2",
      static_cast<int>(panel_x + 25),
      y,
      16,
      kText);

  DrawText(
      "Reset view",
      static_cast<int>(panel_x + 130),
      y,
      16,
      kMutedText);

  y += 27;

  DrawText(
      "Middle mouse",
      static_cast<int>(panel_x + 25),
      y,
      16,
      kText);

  DrawText(
      "Pan",
      static_cast<int>(panel_x + 130),
      y,
      16,
      kMutedText);

  y += 27;

  DrawText(
      "Mouse wheel",
      static_cast<int>(panel_x + 25),
      y,
      16,
      kText);

  DrawText(
      "Zoom",
      static_cast<int>(panel_x + 130),
      y,
      16,
      kMutedText);

  y += 40;

  DrawText(
      "FILE",
      static_cast<int>(panel_x + 20),
      y,
      18,
      kText);

  y += 30;

  Rectangle filename_box = {
      panel_x + 20,
      static_cast<float>(y),
      panel_width - 40,
      38.0f};

  DrawRectangleRec(
      filename_box,
      kBackground);

  DrawRectangleLinesEx(
      filename_box,
      1.0f,
      filename_active_ ? kText : kBorder);

  const char* filename =
      filename_input_.empty()
          ? "Type filename and press Enter"
          : filename_input_.c_str();

  Color filename_color =
      filename_input_.empty()
          ? kMutedText
          : kText;

  DrawText(
      filename,
      static_cast<int>(filename_box.x + 10),
      static_cast<int>(filename_box.y + 10),
      16,
      filename_color);

  DrawText(
      "F1 / H: close help",
      static_cast<int>(panel_x + 20),
      static_cast<int>(
          panel_y + panel_height - 35),
      14,
      kMutedText);
}

void Renderer::FitView(
    const std::vector<Point>& points,
    const std::vector<Centroid>& centroids) {
  if (points.empty() && centroids.empty()) {
    zoom_ = 1.0f;
    pan_ = {0.0f, 0.0f};
    return;
  }

  double minimum_x =
      std::numeric_limits<double>::max();

  double maximum_x =
      std::numeric_limits<double>::lowest();

  double minimum_y =
      std::numeric_limits<double>::max();

  double maximum_y =
      std::numeric_limits<double>::lowest();

  for (const Point& point : points) {
    minimum_x =
        std::min(minimum_x, point.GetX());

    maximum_x =
        std::max(maximum_x, point.GetX());

    minimum_y =
        std::min(minimum_y, point.GetY());

    maximum_y =
        std::max(maximum_y, point.GetY());
  }

  for (const Centroid& centroid : centroids) {
    minimum_x =
        std::min(minimum_x, centroid.GetX());

    maximum_x =
        std::max(maximum_x, centroid.GetX());

    minimum_y =
        std::min(minimum_y, centroid.GetY());

    maximum_y =
        std::max(maximum_y, centroid.GetY());
  }

  if (maximum_x <= minimum_x ||
      maximum_y <= minimum_y) {
    zoom_ = 1.0f;
    pan_ = {0.0f, 0.0f};
    return;
  }

  double data_width =
      maximum_x - minimum_x;

  double data_height =
      maximum_y - minimum_y;

  double world_width =
      world_max_ - world_min_;

  double world_height =
      world_max_ - world_min_;

  if (world_width <= 0.0 ||
      world_height <= 0.0) {
    return;
  }

  float plot_width =
      static_cast<float>(
          screen_width_ - kLeftPanelWidth);

  float plot_height =
      static_cast<float>(
          screen_height_ -
          kTopBarHeight -
          kBottomBarHeight);

  float zoom_x =
      plot_width /
      static_cast<float>(
          data_width / world_width *
          plot_width);

  float zoom_y =
      plot_height /
      static_cast<float>(
          data_height / world_height *
          plot_height);

  zoom_ =
      std::min(zoom_x, zoom_y) * 0.9f;

  zoom_ =
      ClampFloat(
          zoom_,
          0.1f,
          20.0f);

  Vector2 data_center = {
      static_cast<float>(
          (minimum_x + maximum_x) / 2.0),
      static_cast<float>(
          (minimum_y + maximum_y) / 2.0)};

  Vector2 screen_center = {
      kLeftPanelWidth +
          (screen_width_ -
           kLeftPanelWidth) /
              2.0f,
      kTopBarHeight +
          (screen_height_ -
           kTopBarHeight -
           kBottomBarHeight) /
              2.0f};

  Vector2 current_center =
      WorldToScreen(data_center);

  pan_.x +=
      screen_center.x -
      current_center.x;

  pan_.y +=
      screen_center.y -
      current_center.y;
}