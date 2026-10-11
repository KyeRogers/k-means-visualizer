#include "renderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {

const Color kBackground = {18, 24, 38, 255};
const Color kSurface = {29, 38, 58, 255};
const Color kSurfaceLight = {39, 51, 76, 255};
const Color kMap = {12, 18, 30, 255};
const Color kText = {236, 241, 248, 255};
const Color kMuted = {156, 170, 191, 255};
const Color kAccent = {74, 174, 255, 255};
const Color kGood = {83, 205, 145, 255};
const Color kWarning = {245, 184, 77, 255};
const Color kBorder = {67, 82, 109, 255};

float Clamp(float value, float low, float high) {
  return std::max(low, std::min(value, high));
}

}  // namespace

void Renderer::Initialize(double world_min, double world_max) {
  world_min_ = world_min;
  world_max_ = world_max;
  screen_ = Screen::Welcome;
  help_ = false;
  ResetView();
  InitWindow(1280, 800, "K-Means Studio");
  SetExitKey(KEY_Q);
  SetTargetFPS(60);
}

bool Renderer::ShouldClose() const { return WindowShouldClose(); }

Renderer::InputEvents Renderer::PollInput(bool converged) {
  InputEvents events;
  screen_width_ = GetScreenWidth();
  screen_height_ = GetScreenHeight();
  if (!converged) summary_dismissed_ = false;

  if (screen_ == Screen::Welcome) {
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
        (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
         CheckCollisionPointRec(
             GetMousePosition(),
             {static_cast<float>(screen_width_ / 2 - 120),
              static_cast<float>(screen_height_ - 120), 240, 48}))) {
      screen_ = Screen::Setup;
    }
    return events;
  }

  // Filename entry is modal: do not let typed characters trigger simulator
  // shortcuts or interact with controls behind the input.
  if (filename_active_) {
    ReadTextInput(filename_input_, false, filename_active_);
    if (IsKeyPressed(KEY_ESCAPE)) {
      filename_active_ = false;
      filename_input_.clear();
    } else if (IsKeyPressed(KEY_ENTER) && !filename_input_.empty()) {
      events.load_file = true;
      events.load_filename = filename_input_;
      filename_active_ = false;
      filename_input_.clear();
    }
    return events;
  }

  if (IsKeyPressed(KEY_F1) || IsKeyPressed(KEY_H)) {
    help_ = !help_;
  }
  if (help_) {
    if (IsKeyPressed(KEY_ESCAPE)) {
      help_ = false;
    }
    return events;
  }

  if (screen_ == Screen::Setup) {
    ReadTextInput(seed_input_, true, seed_active_);
    ReadTextInput(dataset_seed_input_, true, dataset_seed_active_);
    ReadTextInput(dataset_deviation_x_input_, true,
                  dataset_deviation_x_active_);
    ReadTextInput(dataset_deviation_y_input_, true,
                  dataset_deviation_y_active_);
    ReadTextInput(dataset_ring_radius_input_, true,
                  dataset_ring_radius_active_);
    if (IsKeyPressed(KEY_ESCAPE) && !seed_active_) {
      events.main_menu = true;
      return events;
    }
    if (IsKeyPressed(KEY_G)) {
      events.generate_dataset = true;
      events.dataset_points = selected_dataset_points_;
      events.dataset_type = selected_dataset_type_;
      events.dataset_blobs = selected_dataset_blobs_;
      events.dataset_deviation_x = selected_dataset_deviation_x_;
      events.dataset_deviation_y = selected_dataset_deviation_y_;
      events.dataset_ring_radius = selected_dataset_ring_radius_;
      events.dataset_seed = GetRandomValue(-1000000000, 1000000000);
    }
    if (IsKeyPressed(KEY_P)) {
      selected_seed_ = GetRandomValue(-1000000000, 1000000000);
      seed_input_ = std::to_string(selected_seed_);
      events.seed_changed = true;
      events.seed = selected_seed_;
    }
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
      selected_k_ = std::max(kMinK, selected_k_ - 1);
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
      selected_k_ = std::min(kMaxK, selected_k_ + 1);
    }
    if (IsKeyPressed(KEY_ONE))
      selected_method_ = CentroidInitialization::Randomized;
    if (IsKeyPressed(KEY_TWO))
      selected_method_ = CentroidInitialization::KMeansPlusPlus;
    if (IsKeyPressed(KEY_THREE))
      selected_method_ = CentroidInitialization::Manual;
    if (IsKeyPressed(KEY_FOUR)) selected_dataset_type_ = DatasetType::Random;
    if (IsKeyPressed(KEY_FIVE)) selected_dataset_type_ = DatasetType::Blobs;
    if (IsKeyPressed(KEY_SIX))
      selected_dataset_type_ = DatasetType::ElongatedClusters;
    if (IsKeyPressed(KEY_SEVEN)) selected_dataset_type_ = DatasetType::TwoMoons;
    if (IsKeyPressed(KEY_EIGHT)) selected_dataset_type_ = DatasetType::Ring;
    if (IsKeyPressed(KEY_UP))
      selected_dataset_points_ = std::min(1000, selected_dataset_points_ + 10);
    if (IsKeyPressed(KEY_DOWN))
      selected_dataset_points_ = std::max(10, selected_dataset_points_ - 10);
    const bool blobs =
        selected_dataset_type_ == DatasetType::Blobs ||
        selected_dataset_type_ == DatasetType::ElongatedClusters;
    if (blobs && IsKeyPressed(KEY_COMMA))
      selected_dataset_blobs_ = std::max(1, selected_dataset_blobs_ - 1);
    if (blobs && IsKeyPressed(KEY_PERIOD))
      selected_dataset_blobs_ = std::min(12, selected_dataset_blobs_ + 1);
    const bool needs_deviation =
        selected_dataset_type_ != DatasetType::Random;
    const bool elongated =
        selected_dataset_type_ == DatasetType::ElongatedClusters;
    const bool ring_or_moons =
        selected_dataset_type_ == DatasetType::Ring ||
        selected_dataset_type_ == DatasetType::TwoMoons;
    if (needs_deviation && IsKeyPressed(KEY_J))
      selected_dataset_deviation_x_ =
          std::max(1, selected_dataset_deviation_x_ - 5);
    if (needs_deviation && IsKeyPressed(KEY_K))
      selected_dataset_deviation_x_ =
          std::min(200, selected_dataset_deviation_x_ + 5);
    if (elongated && IsKeyPressed(KEY_U))
      selected_dataset_deviation_y_ =
          std::max(1, selected_dataset_deviation_y_ - 5);
    if (elongated && IsKeyPressed(KEY_I))
      selected_dataset_deviation_y_ =
          std::min(200, selected_dataset_deviation_y_ + 5);
    if (ring_or_moons && IsKeyPressed(KEY_N))
      selected_dataset_ring_radius_ =
          std::max(1, selected_dataset_ring_radius_ - 10);
    if (ring_or_moons && IsKeyPressed(KEY_B))
      selected_dataset_ring_radius_ =
          std::min(499, selected_dataset_ring_radius_ + 10);
    if (!dataset_deviation_x_active_)
      dataset_deviation_x_input_ = std::to_string(selected_dataset_deviation_x_);
    if (!dataset_deviation_y_active_)
      dataset_deviation_y_input_ = std::to_string(selected_dataset_deviation_y_);
    if (!dataset_ring_radius_active_)
      dataset_ring_radius_input_ =
          std::to_string(selected_dataset_ring_radius_);

    auto commit_seed = [&]() {
      char* end = nullptr;
      long value = std::strtol(seed_input_.c_str(), &end, 10);
      if (end != seed_input_.c_str() && *end == '\0') {
        selected_seed_ = static_cast<int>(value);
        events.seed_changed = true;
        events.seed = selected_seed_;
      }
      seed_input_ = std::to_string(selected_seed_);
      seed_active_ = false;
    };
    if (seed_active_ && IsKeyPressed(KEY_ENTER)) commit_seed();
    if (dataset_seed_active_ && IsKeyPressed(KEY_ENTER)) {
      char* end = nullptr;
      long value = std::strtol(dataset_seed_input_.c_str(), &end, 10);
      if (end != dataset_seed_input_.c_str() && *end == '\0') {
        selected_dataset_seed_ = static_cast<int>(value);
        events.dataset_seed_changed = true;
        events.dataset_seed = selected_dataset_seed_;
      }
      dataset_seed_input_ = std::to_string(selected_dataset_seed_);
      dataset_seed_active_ = false;
    }
    auto commit_deviation = [](std::string& input, int& selected,
                               bool& active) {
      char* end = nullptr;
      long value = std::strtol(input.c_str(), &end, 10);
      if (end != input.c_str() && *end == '\0')
        selected = std::clamp(static_cast<int>(value), 1, 200);
      input = std::to_string(selected);
      active = false;
    };
    if (dataset_deviation_x_active_ &&
        IsKeyPressed(KEY_ENTER))
      commit_deviation(dataset_deviation_x_input_,
                       selected_dataset_deviation_x_,
                       dataset_deviation_x_active_);
    if (dataset_deviation_y_active_ &&
        IsKeyPressed(KEY_ENTER))
      commit_deviation(dataset_deviation_y_input_,
                       selected_dataset_deviation_y_,
                       dataset_deviation_y_active_);
    if (dataset_ring_radius_active_ && IsKeyPressed(KEY_ENTER)) {
      char* end = nullptr;
      long value = std::strtol(dataset_ring_radius_input_.c_str(), &end, 10);
      if (end != dataset_ring_radius_input_.c_str() && *end == '\0')
        selected_dataset_ring_radius_ =
            std::clamp(static_cast<int>(value), 1, 499);
      dataset_ring_radius_input_ = std::to_string(selected_dataset_ring_radius_);
      dataset_ring_radius_active_ = false;
    }

    if (IsKeyPressed(KEY_ENTER) && !seed_active_ &&
        !dataset_seed_active_ && !dataset_deviation_x_active_ &&
        !dataset_deviation_y_active_ && !dataset_ring_radius_active_) {
      if (selected_method_ == CentroidInitialization::Manual) {
        manual_centroids_.clear();
        screen_ = Screen::Manual;
      } else {
        events.k_selected = true;
        events.k = selected_k_;
        events.initialization_method = selected_method_;
        screen_ = Screen::Simulation;
      }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 mouse = GetMousePosition();
      Rectangle seed_box = {300, 170, 220, 38};
      Rectangle random_seed = {530, 170, 145, 38};
      Rectangle dataset_seed_box = {700, 170, 220, 38};
      Rectangle random_data = {40, static_cast<float>(screen_height_ - 86), 190,
                               40};
      Rectangle start = {static_cast<float>(screen_width_ - 260), 190, 210, 44};
      Rectangle minus = {40, 170, 52, 44};
      Rectangle plus = {170, 170, 52, 44};
      Rectangle random_card = {40, 340, 195, 90};
      Rectangle plus_card = {255, 340, 195, 90};
      Rectangle manual_card = {470, 340, 195, 90};
      Rectangle data_random = {40, 500, 160, 64};
      Rectangle data_blobs = {210, 500, 160, 64};
      Rectangle data_elongated = {380, 500, 190, 64};
      Rectangle data_moons = {580, 500, 160, 64};
      Rectangle data_ring = {750, 500, 140, 64};
      Rectangle blob_minus = {40, 640, 42, 32};
      Rectangle blob_plus = {140, 640, 42, 32};
      Rectangle deviation_x_box = {310, 640, 90, 32};
      Rectangle deviation_y_box = {500, 640, 90, 32};
      Rectangle ring_radius_box = {700, 640, 180, 32};
      if (CheckCollisionPointRec(mouse, seed_box)) {
        seed_active_ = true;
      } else if (CheckCollisionPointRec(mouse, dataset_seed_box)) {
        dataset_seed_active_ = true;
      } else if (CheckCollisionPointRec(mouse, random_seed)) {
        selected_seed_ = GetRandomValue(-1000000000, 1000000000);
        seed_input_ = std::to_string(selected_seed_);
        events.seed_changed = true;
        events.seed = selected_seed_;
      } else if (CheckCollisionPointRec(mouse, minus)) {
        selected_k_ = std::max(kMinK, selected_k_ - 1);
      } else if (CheckCollisionPointRec(mouse, plus)) {
        selected_k_ = std::min(kMaxK, selected_k_ + 1);
      } else if (CheckCollisionPointRec(mouse, random_card)) {
        selected_method_ = CentroidInitialization::Randomized;
      } else if (CheckCollisionPointRec(mouse, plus_card)) {
        selected_method_ = CentroidInitialization::KMeansPlusPlus;
      } else if (CheckCollisionPointRec(mouse, manual_card)) {
        selected_method_ = CentroidInitialization::Manual;
      } else if (CheckCollisionPointRec(mouse, data_random)) {
        selected_dataset_type_ = DatasetType::Random;
      } else if (CheckCollisionPointRec(mouse, data_blobs)) {
        selected_dataset_type_ = DatasetType::Blobs;
      } else if (CheckCollisionPointRec(mouse, data_elongated)) {
        selected_dataset_type_ = DatasetType::ElongatedClusters;
      } else if (CheckCollisionPointRec(mouse, data_moons)) {
        selected_dataset_type_ = DatasetType::TwoMoons;
      } else if (CheckCollisionPointRec(mouse, data_ring)) {
        selected_dataset_type_ = DatasetType::Ring;
      } else if ((selected_dataset_type_ == DatasetType::Blobs ||
                  selected_dataset_type_ == DatasetType::ElongatedClusters) &&
                 CheckCollisionPointRec(mouse, blob_minus)) {
        selected_dataset_blobs_ = std::max(1, selected_dataset_blobs_ - 1);
      } else if ((selected_dataset_type_ == DatasetType::Blobs ||
                  selected_dataset_type_ == DatasetType::ElongatedClusters) &&
                 CheckCollisionPointRec(mouse, blob_plus)) {
        selected_dataset_blobs_ = std::min(12, selected_dataset_blobs_ + 1);
      } else if (needs_deviation &&
                 CheckCollisionPointRec(mouse, deviation_x_box)) {
        dataset_deviation_x_active_ = true;
      } else if (selected_dataset_type_ == DatasetType::ElongatedClusters &&
                 CheckCollisionPointRec(mouse, deviation_y_box)) {
        dataset_deviation_y_active_ = true;
      } else if ((selected_dataset_type_ == DatasetType::Ring ||
                  selected_dataset_type_ == DatasetType::TwoMoons) &&
                 CheckCollisionPointRec(mouse, ring_radius_box)) {
        dataset_ring_radius_active_ = true;
      } else if (CheckCollisionPointRec(mouse, random_data)) {
        events.generate_dataset = true;
        events.dataset_points = selected_dataset_points_;
        events.dataset_type = selected_dataset_type_;
        events.dataset_blobs = selected_dataset_blobs_;
        events.dataset_deviation_x = selected_dataset_deviation_x_;
        events.dataset_deviation_y = selected_dataset_deviation_y_;
        events.dataset_ring_radius = selected_dataset_ring_radius_;
        events.dataset_seed = GetRandomValue(-1000000000, 1000000000);
      } else if (CheckCollisionPointRec(mouse, start)) {
        if (selected_method_ == CentroidInitialization::Manual) {
          manual_centroids_.clear();
          screen_ = Screen::Manual;
        } else {
          events.k_selected = true;
          events.k = selected_k_;
          events.initialization_method = selected_method_;
          screen_ = Screen::Simulation;
        }
      }
    }
    return events;
  }

  if (screen_ == Screen::Manual) {
    if (IsKeyPressed(KEY_ESCAPE)) {
      screen_ = Screen::Setup;
      manual_centroids_.clear();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && !manual_centroids_.empty()) {
      manual_centroids_.pop_back();
    }
    if (IsKeyPressed(KEY_R)) manual_centroids_.clear();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && InMap(GetMousePosition()) &&
        static_cast<int>(manual_centroids_.size()) < selected_k_) {
      Vector2 world = ScreenToWorld(GetMousePosition());
      manual_centroids_.push_back(world);
    }
    if (IsKeyPressed(KEY_ENTER) &&
        static_cast<int>(manual_centroids_.size()) == selected_k_) {
      events.k_selected = true;
      events.k = selected_k_;
      events.initialization_method = CentroidInitialization::Manual;
      events.manual_centroids = manual_centroids_;
      screen_ = Screen::Simulation;
    }
    return events;
  }

  if (screen_ == Screen::Results) {
    if (IsKeyPressed(KEY_UP)) result_scroll_ = std::max(0, result_scroll_ - 1);
    if (IsKeyPressed(KEY_DOWN))
      result_scroll_ =
          std::min(std::max(0, static_cast<int>(result_costs_.size()) - 15),
                   result_scroll_ + 1);
    const float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
      const int max_scroll =
          std::max(0, static_cast<int>(result_costs_.size()) - 15);
      result_scroll_ =
          std::clamp(result_scroll_ + (wheel < 0.0f ? 3 : -3), 0, max_scroll);
    }
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_M)) {
      events.main_menu = true;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Rectangle again = {32, static_cast<float>(screen_height_ - 70), 140, 38};
      Rectangle menu = {184, static_cast<float>(screen_height_ - 70), 140, 38};
      Vector2 mouse = GetMousePosition();
      if (CheckCollisionPointRec(mouse, again)) {
        events.reset = true;
        screen_ = Screen::Simulation;
      }
      if (CheckCollisionPointRec(mouse, menu)) events.main_menu = true;
    }
    return events;
  }

  if (screen_ == Screen::Summary) {
    if (IsKeyPressed(KEY_ENTER)) {
      screen_ = Screen::Simulation;
      summary_dismissed_ = true;
    }
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_M))
      events.main_menu = true;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 mouse = GetMousePosition();
      if (CheckCollisionPointRec(
              mouse, {32, static_cast<float>(screen_height_ - 70), 150, 38})) {
        events.reset = true;
        screen_ = Screen::Simulation;
      } else if (CheckCollisionPointRec(
                     mouse,
                     {194, static_cast<float>(screen_height_ - 70), 150, 38})) {
        events.main_menu = true;
      }
    }
    return events;
  }

  if (screen_ == Screen::MultiRun) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_M))
      screen_ = Screen::Simulation;
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))
      selected_runs_ = std::max(kMinRuns, selected_runs_ - 1);
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))
      selected_runs_ = std::min(kMaxRuns, selected_runs_ + 1);
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
      events.multi_run_requested = true;
      events.total_runs = selected_runs_;
      screen_ = Screen::Simulation;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Rectangle run = {static_cast<float>(screen_width_ / 2 - 105), 390, 210,
                       44};
      if (CheckCollisionPointRec(GetMousePosition(), run)) {
        events.multi_run_requested = true;
        events.total_runs = selected_runs_;
        screen_ = Screen::Simulation;
      }
    }
    return events;
  }

  if (converged && screen_ == Screen::Simulation && !summary_dismissed_) {
    screen_ = Screen::Summary;
    return events;
  }

  if (IsKeyPressed(KEY_G)) {
    events.generate_dataset = true;
    events.dataset_points = selected_dataset_points_;
    events.dataset_type = selected_dataset_type_;
    events.dataset_blobs = selected_dataset_blobs_;
    events.dataset_deviation_x = selected_dataset_deviation_x_;
    events.dataset_deviation_y = selected_dataset_deviation_y_;
    events.dataset_ring_radius = selected_dataset_ring_radius_;
    events.dataset_seed = GetRandomValue(-1000000000, 1000000000);
  }
  if (IsKeyPressed(KEY_SPACE)) events.step = true;
  if (IsKeyPressed(KEY_R)) events.toggle_running = true;
  if (IsKeyPressed(KEY_ESCAPE)) events.reset = true;
  if (IsKeyPressed(KEY_M)) {
    screen_ = Screen::MultiRun;
    return events;
  }
  if (IsKeyPressed(KEY_S)) {
    events.hard_reset_requested = true;
    hard_reset_ = true;
    screen_ = Screen::Setup;
    return events;
  }
  if (IsKeyPressed(KEY_L)) filename_active_ = true;
  if (IsKeyPressed(KEY_F2)) ResetView();
  if (IsKeyPressed(KEY_LEFT_BRACKET)) events.speed_change = -1;
  if (IsKeyPressed(KEY_RIGHT_BRACKET)) events.speed_change = 1;

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    Vector2 mouse = GetMousePosition();
    const float y = static_cast<float>(screen_height_ - 62);
    Rectangle step = {24, y, 100, 38};
    Rectangle run = {132, y, 120, 38};
    Rectangle reset = {260, y, 110, 38};
    Rectangle multi = {378, y, 125, 38};
    Rectangle data = {511, y, 130, 38};
    Rectangle load = {653, y, 100, 38};
    Rectangle slower = {765, y, 38, 38};
    Rectangle faster = {811, y, 38, 38};
    Rectangle setup = {static_cast<float>(screen_width_ - 142), 14, 118, 34};
    if (CheckCollisionPointRec(mouse, step))
      events.step = true;
    else if (CheckCollisionPointRec(mouse, run))
      events.toggle_running = true;
    else if (CheckCollisionPointRec(mouse, reset))
      events.reset = true;
    else if (CheckCollisionPointRec(mouse, multi))
      screen_ = Screen::MultiRun;
    else if (CheckCollisionPointRec(mouse, data)) {
      events.generate_dataset = true;
      events.dataset_points = selected_dataset_points_;
      events.dataset_type = selected_dataset_type_;
      events.dataset_blobs = selected_dataset_blobs_;
      events.dataset_deviation_x = selected_dataset_deviation_x_;
      events.dataset_deviation_y = selected_dataset_deviation_y_;
      events.dataset_ring_radius = selected_dataset_ring_radius_;
      events.dataset_seed = GetRandomValue(-1000000000, 1000000000);
    } else if (CheckCollisionPointRec(mouse, load)) {
      filename_active_ = true;
    } else if (CheckCollisionPointRec(mouse, slower)) {
      events.speed_change = -1;
    } else if (CheckCollisionPointRec(mouse, faster)) {
      events.speed_change = 1;
    } else if (CheckCollisionPointRec(mouse, setup)) {
      events.hard_reset_requested = true;
      screen_ = Screen::Setup;
      hard_reset_ = true;
    } else if (InMap(mouse)) {
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
    Vector2 before = ScreenToWorld(mouse);
    zoom_ = Clamp(zoom_ * (wheel > 0 ? 1.1f : 0.9f), 0.2f, 12.0f);
    Vector2 after = WorldToScreen(before);
    pan_.x += mouse.x - after.x;
    pan_.y += mouse.y - after.y;
  }
  return events;
}

void Renderer::Render(const std::vector<Point>& points,
                      const std::vector<Centroid>& centroids, double cost,
                      int iteration, bool converged,
                      double run_interval_seconds) {
  screen_width_ = GetScreenWidth();
  screen_height_ = GetScreenHeight();
  BeginDrawing();
  ClearBackground(kBackground);
  if (screen_ == Screen::Welcome)
    DrawWelcome();
  else if (screen_ == Screen::Setup)
    DrawSetup();
  else if (screen_ == Screen::Manual)
    DrawManual(points);
  else if (screen_ == Screen::MultiRun) {
    DrawRectangle(0, 0, screen_width_, screen_height_, kBackground);
    DrawText("MULTI-RUN", 32, 30, 34, kText);
    DrawText("Compare independent initializations", 34, 75, 18, kMuted);
    DrawText("Runs", 34, 170, 18, kMuted);
    char text[32];
    std::snprintf(text, sizeof(text), "%d", selected_runs_);
    DrawText(text, 34, 205, 44, kAccent);
    DrawText("Use A/D or Left/Right to change", 34, 270, 16, kMuted);
    DrawButton({static_cast<float>(screen_width_ / 2 - 105), 390, 210, 44},
               "RUN EXPERIMENT", true);
    DrawText("Enter / Space to start    Esc to cancel", 34, screen_height_ - 42,
             16, kMuted);
  } else if (screen_ == Screen::Summary)
    DrawSummary(points, centroids, cost, iteration);
  else if (screen_ == Screen::Results)
    DrawResults();
  else
    DrawDashboard(points, centroids, cost, iteration, converged,
                  run_interval_seconds);
  if (help_) DrawHelp();
  EndDrawing();
}

void Renderer::Close() {
  if (IsWindowReady()) CloseWindow();
}

void Renderer::DrawWelcome() const {
  DrawRectangle(0, 0, screen_width_, screen_height_, kBackground);
  DrawCircle(static_cast<float>(screen_width_ - 130), 135, 92,
             Fade(kAccent, 0.12f));
  DrawCircle(static_cast<float>(screen_width_ - 130), 135, 58,
             Fade(kGood, 0.18f));
  DrawCircle(static_cast<float>(screen_width_ - 130), 135, 25, kAccent);
  DrawLineEx({static_cast<float>(screen_width_ - 245), 135},
             {static_cast<float>(screen_width_ - 15), 135}, 2,
             Fade(kAccent, 0.35f));
  DrawLineEx({static_cast<float>(screen_width_ - 130), 20},
             {static_cast<float>(screen_width_ - 130), 250}, 2,
             Fade(kGood, 0.35f));

  DrawText("K-MEANS STUDIO", 70, 78, 42, kText);
  DrawText("Explore how clusters emerge from data", 74, 132, 20, kMuted);

  DrawRectangle(70, 220, 500, 170, kSurface);
  DrawRectangleLinesEx({70, 220, 500, 170}, 1, kBorder);
  DrawText("WHAT IS K-MEANS?", 98, 248, 18, kAccent);
  DrawText("K-means groups points into K clusters by placing", 98, 290, 17,
           kText);
  DrawText("centroids and moving them toward their assigned data.", 98, 318, 17,
           kText);
  DrawText("It is useful for finding structure in data at a glance.", 98, 346,
           17, kMuted);

  DrawRectangle(610, 300, 500, 170, kSurface);
  DrawRectangleLinesEx({610, 300, 500, 170}, 1, kBorder);
  DrawText("WHAT YOU CAN DO", 638, 328, 18, kGood);
  DrawText("Generate or load datasets, tune their shape, place", 638, 370, 17,
           kText);
  DrawText("centroids, step through runs, and compare multiple results.", 638,
           398, 17, kText);
  DrawText("Everything is visual and interactive.", 638, 426, 17, kMuted);

  DrawButton({static_cast<float>(screen_width_ / 2 - 120),
              static_cast<float>(screen_height_ - 120), 240, 48},
             "CONTINUE (ENTER)", true);
  DrawText("Press Enter or click to begin", screen_width_ / 2 - 122,
           screen_height_ - 58, 16, kMuted);
}

void Renderer::DrawSetup() const {
  DrawRectangle(0, 0, screen_width_, screen_height_, kBackground);
  DrawText(hard_reset_ ? "CHANGE CLUSTERING" : "NEW K-MEANS RUN", 40, 36, 38,
           kText);
  DrawText("Configure the run, then start. The map stays focused on your data.",
           42, 84, 17, kMuted);

  DrawText("CLUSTERS", 40, 145, 16, kMuted);
  DrawButton({40, 170, 52, 44}, "-", selected_k_ == kMinK);
  char value[32];
  std::snprintf(value, sizeof(value), "%d", selected_k_);
  DrawText(value, 122, 174, 36, kText);
  DrawButton({170, 170, 52, 44}, "+", selected_k_ == kMaxK);
  DrawText("A/D or Left/Right", 40, 224, 14, kMuted);

  DrawText("SEED", 300, 145, 16, kMuted);
  DrawTextInput({300, 170, 220, 38}, seed_input_, seed_active_);
  DrawButton({530, 170, 145, 38}, "RANDOMIZE (P)");
  DrawText("POINT SEED", 700, 145, 16, kMuted);
  DrawTextInput({700, 170, 220, 38}, dataset_seed_input_, dataset_seed_active_);

  DrawText("INITIALIZATION", 40, 310, 16, kMuted);
  const char* names[] = {"RANDOM", "K-MEANS++", "MANUAL"};
  for (int i = 0; i < 3; ++i) {
    Rectangle card = {40.0f + i * 215.0f, 340, 195, 90};
    DrawButton(card, names[i],
               selected_method_ == static_cast<CentroidInitialization>(i));
  }
  DrawText("1", 52, 350, 14, kMuted);
  DrawText("2", 267, 350, 14, kMuted);
  DrawText("3", 482, 350, 14, kMuted);

  DrawText("DATASET  (4-8)", 40, 475, 16, kMuted);
  const char* dataset_names[] = {"RANDOM", "BLOBS", "ELONGATED", "TWO MOONS",
                                 "RING"};
  const DatasetType dataset_types[] = {
      DatasetType::Random, DatasetType::Blobs, DatasetType::ElongatedClusters,
      DatasetType::TwoMoons, DatasetType::Ring};
  const float dataset_widths[] = {160, 160, 190, 160, 140};
  float dataset_x = 40;
  for (int i = 0; i < 5; ++i) {
    Rectangle card = {dataset_x, 500, dataset_widths[i], 64};
    DrawButton(card, dataset_names[i],
               selected_dataset_type_ == dataset_types[i]);
    DrawText(std::to_string(i + 4).c_str(), static_cast<int>(card.x + 10), 510,
             14, kMuted);
    dataset_x += dataset_widths[i] + 10;
  }
  std::snprintf(value, sizeof(value), "POINTS: %d  (Up/Down)",
                selected_dataset_points_);
  DrawText(value, 40, 590, 16, kText);
  const bool random = selected_dataset_type_ == DatasetType::Random;
  const bool blobs = selected_dataset_type_ == DatasetType::Blobs ||
                     selected_dataset_type_ == DatasetType::ElongatedClusters;
  const bool elongated =
      selected_dataset_type_ == DatasetType::ElongatedClusters;
  const bool ring = selected_dataset_type_ == DatasetType::Ring;
  const bool moons = selected_dataset_type_ == DatasetType::TwoMoons;
  const bool ring_or_moons = ring || moons;
  if (blobs) {
    DrawText("BLOBS", 40, 618, 14, kMuted);
    DrawButton({40, 640, 42, 32}, "-");
    std::snprintf(value, sizeof(value), "%d", selected_dataset_blobs_);
    DrawText(value, 100, 644, 20, kText);
    DrawButton({140, 640, 42, 32}, "+");
    DrawText("[, ] or buttons", 195, 648, 14, kMuted);
  }
  const bool needs_deviation = !random;
  if (needs_deviation) {
    DrawText(ring ? "STD DEV (MAX THICKNESS)"
                  : (elongated ? "STD DEV X / Y" : "STD DEV"),
             310, 618, 14, kMuted);
    DrawTextInput({310, 640, 90, 32}, dataset_deviation_x_input_,
                  dataset_deviation_x_active_);
    DrawText("J/K", 405, 648, 14, kMuted);
  }
  if (elongated) {
    DrawTextInput({500, 640, 90, 32}, dataset_deviation_y_input_,
                  dataset_deviation_y_active_);
    DrawText("Y", 595, 648, 14, kMuted);
    DrawText("U/I", 620, 648, 14, kMuted);
  }
  if (ring_or_moons) {
    DrawText(moons ? "MOON RADIUS" : "RING RADIUS", 700, 618, 14, kMuted);
    DrawTextInput({700, 640, 180, 32}, dataset_ring_radius_input_,
                  dataset_ring_radius_active_);
    DrawText(moons ? "average radius (N/B)" : "average radius (N/B)", 700,
             678, 14, kMuted);
    if (ring)
      DrawText("thickness = STD DEV (max)", 900, 648, 14, kAccent);
    else
      DrawText("spread = STD DEV", 900, 648, 14, kAccent);
  }

  DrawButton({static_cast<float>(screen_width_ - 260), 190, 210, 44},
             selected_method_ == CentroidInitialization::Manual
                 ? "PLACE CENTROIDS (ENTER)"
                 : "START RUN (ENTER)",
             true);
  DrawButton({40, static_cast<float>(screen_height_ - 86), 190, 40},
             "NEW RANDOM DATA (G)");
  std::snprintf(value, sizeof(value), "%d points  (G)",
                selected_dataset_points_);
  DrawText(value, 245, screen_height_ - 74, 16, kMuted);
  DrawText("Enter: start    G: regenerate data    F1: help    Q: quit", 40,
           screen_height_ - 38, 16, kMuted);
}

void Renderer::DrawManual(const std::vector<Point>& points) {
  DrawRectangle(0, 0, screen_width_, screen_height_, kBackground);
  DrawText("PLACE CENTROIDS", 28, 24, 30, kText);
  char title[64];
  std::snprintf(title, sizeof(title), "%d / %d placed",
                static_cast<int>(manual_centroids_.size()), selected_k_);
  DrawText(title, 30, 65, 17, kMuted);
  DrawMap(points, {});
  for (std::size_t i = 0; i < manual_centroids_.size(); ++i) {
    Vector2 p = WorldToScreen(manual_centroids_[i]);
    Color color = ClusterColor(static_cast<int>(i));
    DrawCircleLinesV(p, 15, color);
    DrawLineEx({p.x - 10, p.y}, {p.x + 10, p.y}, 3, color);
    DrawLineEx({p.x, p.y - 10}, {p.x, p.y + 10}, 3, color);
  }
  DrawText("Click the map to place. Backspace removes one. R clears all.", 28,
           screen_height_ - 58, 16, kMuted);
  DrawText("Enter: use centroids    Esc: back", screen_width_ - 330,
           screen_height_ - 58, 16, kText);
}

void Renderer::DrawDashboard(const std::vector<Point>& points,
                             const std::vector<Centroid>& centroids,
                             double cost, int iteration, bool converged,
                             double interval) {
  DrawRectangle(0, 0, screen_width_, screen_height_, kBackground);
  DrawRectangle(0, 0, screen_width_, 64, kSurface);
  DrawText("K-MEANS", 24, 17, 28, kText);
  char text[128];
  std::snprintf(text, sizeof(text), "K %d    ITERATION %d    COST %.3f",
                static_cast<int>(centroids.size()), iteration, cost);
  DrawText(text, 220, 23, 17, kMuted);
  DrawText(converged ? "CONVERGED" : "RUNNING", screen_width_ - 235, 23, 17,
           converged ? kGood : kAccent);
  DrawButton({static_cast<float>(screen_width_ - 142), 14, 118, 34},
             "SETUP (S)");

  DrawMap(points, centroids);

  DrawRectangle(0, screen_height_ - 62, screen_width_, 62, kSurface);
  const float y = static_cast<float>(screen_height_ - 52);
  DrawButton({24, y, 100, 38}, "STEP (SPACE)");
  DrawButton({132, y, 120, 38}, "RUN (R)");
  DrawButton({260, y, 110, 38}, "RESET (ESC)");
  DrawButton({378, y, 125, 38}, "MULTI-RUN (M)");
  DrawButton({511, y, 130, 38}, "NEW DATA (G)");
  DrawButton({653, y, 100, 38}, "LOAD (L)");
  DrawButton({765, y, 38, 38}, "- [");
  DrawButton({811, y, 38, 38}, "+ ]");
  std::snprintf(text, sizeof(text), "%.2gs/step", interval);
  DrawText(text, 860, screen_height_ - 45, 16, kMuted);

  if (!empty_centroids_.empty()) {
    std::snprintf(text, sizeof(text), "Centroid reinitialized: %d",
                  empty_centroids_.front());
    DrawRectangle(24, 78, 270, 34, Fade(kWarning, 0.18f));
    DrawText(text, 36, 88, 15, kWarning);
  }
  if (filename_active_) {
    DrawRectangle(0, 0, screen_width_, screen_height_, Fade(BLACK, 0.7f));
    DrawText("LOAD POINT FILE", 42, 190, 28, kText);
    DrawText("Enter a path containing x y pairs", 42, 232, 16, kMuted);
    DrawTextInput({42, 270, 500, 42}, filename_input_, true);
    DrawText("Enter: load    Esc: cancel", 42, 335, 16, kMuted);
  }
}

void Renderer::DrawResults() {
  DrawRectangle(0, 0, screen_width_, screen_height_, kBackground);
  DrawText("MULTI-RUN RESULTS", 34, 30, 34, kText);
  char text[128];
  std::snprintf(text, sizeof(text), "%d independent runs",
                static_cast<int>(result_costs_.size()));
  DrawText(text, 36, 80, 18, kMuted);
  DrawMap(result_points_, result_centroids_);
  if (best_run_ >= 0) {
    std::snprintf(text, sizeof(text), "Best run: %d    cost: %.3f",
                  best_run_ + 1, result_costs_[best_run_]);
    DrawText(text, 36, 125, 22, kGood);
  }

  DrawRectangle(32, 180, 520, 430, kSurface);
  DrawText("RUN", 54, 202, 15, kMuted);
  DrawText("COST", 180, 202, 15, kMuted);
  const int first_run = std::min(
      result_scroll_, std::max(0, static_cast<int>(result_costs_.size()) - 15));
  const int rows =
      std::min(15, static_cast<int>(result_costs_.size()) - first_run);
  for (int i = 0; i < rows; ++i) {
    const int run = first_run + i;
    std::snprintf(text, sizeof(text), "%3d", run + 1);
    DrawText(text, 54, 235 + i * 24, 16, run == best_run_ ? kGood : kText);
    std::snprintf(text, sizeof(text), "%.4f", result_costs_[run]);
    DrawText(text, 180, 235 + i * 24, 16, run == best_run_ ? kGood : kText);
  }
  DrawText("BEST CLUSTERING MAP", 610, 92, 19, kMuted);
  std::snprintf(text, sizeof(text), "Showing runs %d-%d of %d", first_run + 1,
                first_run + rows, static_cast<int>(result_costs_.size()));
  DrawText(text, 610, 120, 16, kMuted);
  DrawButton({32, static_cast<float>(screen_height_ - 70), 140, 38}, "RESET");
  DrawButton({184, static_cast<float>(screen_height_ - 70), 140, 38}, "MENU");
  DrawText("Enter / Esc", 360, screen_height_ - 48, 16, kMuted);
}

void Renderer::DrawSummary(const std::vector<Point>& points,
                           const std::vector<Centroid>& centroids, double cost,
                           int iteration) {
  DrawRectangle(0, 0, screen_width_, screen_height_, kBackground);
  DrawText("RUN COMPLETE", 32, 24, 34, kGood);
  char text[128];
  std::snprintf(text, sizeof(text), "%d points   %d clusters   %d iterations",
                static_cast<int>(points.size()),
                static_cast<int>(centroids.size()), iteration);
  DrawText(text, 36, 70, 18, kMuted);
  std::snprintf(text, sizeof(text), "Final cost: %.4f", cost);
  DrawText(text, 36, 108, 22, kText);
  DrawMap(points, centroids);
  DrawButton({32, static_cast<float>(screen_height_ - 70), 150, 38},
             "RUN AGAIN", true);
  DrawButton({194, static_cast<float>(screen_height_ - 70), 150, 38},
             "MAIN MENU");
  DrawText("Enter / Esc: menu", 356, screen_height_ - 48, 16, kMuted);
}

void Renderer::DrawMap(const std::vector<Point>& points,
                       const std::vector<Centroid>& centroids) {
  Rectangle map = MapRectangle();
  DrawRectangleRec(map, kMap);
  DrawRectangleLinesEx(map, 1, kBorder);
  BeginScissorMode(static_cast<int>(map.x), static_cast<int>(map.y),
                   static_cast<int>(map.width), static_cast<int>(map.height));
  for (const Point& point : points) {
    Vector2 p = WorldToScreen(
        {static_cast<float>(point.GetX()), static_cast<float>(point.GetY())});
    DrawCircleV(p, 4.5f, ClusterColor(point.GetCentroid() - 1));
  }
  for (std::size_t i = 0; i < centroids.size(); ++i) {
    Vector2 p = WorldToScreen({static_cast<float>(centroids[i].GetX()),
                               static_cast<float>(centroids[i].GetY())});
    Color color = ClusterColor(static_cast<int>(i));
    DrawLineEx({p.x - 11, p.y - 11}, {p.x + 11, p.y + 11}, 4, color);
    DrawLineEx({p.x - 11, p.y + 11}, {p.x + 11, p.y - 11}, 4, color);
  }
  EndScissorMode();
}

void Renderer::DrawHelp() const {
  DrawRectangle(0, 0, screen_width_, screen_height_, Fade(BLACK, 0.72f));
  Rectangle panel = {static_cast<float>(screen_width_ / 2 - 330), 105, 660,
                     560};
  DrawRectangleRec(panel, kSurface);
  DrawRectangleLinesEx(panel, 1, kBorder);
  DrawText("CONTROLS", static_cast<int>(panel.x + 28), 132, 28, kText);
  const char* rows[] = {"Click map",
                        "Add a point",
                        "Middle drag / wheel",
                        "Pan / zoom",
                        "Space",
                        "Step one iteration",
                        "R",
                        "Run or stop",
                        "Esc",
                        "Reset",
                        "G",
                        "Generate fresh random data",
                        "M",
                        "Multi-run comparison",
                        "S",
                        "Open setup",
                        "Manual mode",
                        "Click to place centroids; Backspace removes one",
                        "Q",
                        "Quit"};
  int y = 190;
  for (int i = 0; i < 20; i += 2) {
    DrawText(rows[i], static_cast<int>(panel.x + 30), y, 16, kAccent);
    DrawText(rows[i + 1], static_cast<int>(panel.x + 190), y, 16, kText);
    y += 25;
  }
  DrawText("F1 / H / Esc: close", static_cast<int>(panel.x + 30), 620, 16,
           kMuted);
}

void Renderer::DrawButton(Rectangle rectangle, const char* text,
                          bool selected) const {
  Color fill = selected ? kAccent : kSurfaceLight;
  if (CheckCollisionPointRec(GetMousePosition(), rectangle) && !selected)
    fill = {55, 72, 103, 255};
  DrawRectangleRec(rectangle, fill);
  DrawRectangleLinesEx(rectangle, 1, selected ? kAccent : kBorder);
  int size = 15;
  int width = MeasureText(text, size);
  DrawText(text, static_cast<int>(rectangle.x + (rectangle.width - width) / 2),
           static_cast<int>(rectangle.y + (rectangle.height - size) / 2), size,
           selected ? BLACK : kText);
}

void Renderer::DrawTextInput(Rectangle box, const std::string& value,
                             bool active) const {
  DrawRectangleRec(box, kMap);
  DrawRectangleLinesEx(box, 1, active ? kAccent : kBorder);
  DrawText(value.empty() ? "..." : value.c_str(), static_cast<int>(box.x + 10),
           static_cast<int>(box.y + 10), 16, value.empty() ? kMuted : kText);
}

void Renderer::ReadTextInput(std::string& value, bool integer_only,
                             bool& active) {
  if (!active) return;
  int character = GetCharPressed();
  while (character > 0) {
    bool valid = character >= 32 && character <= 126;
    if (integer_only)
      valid = (character >= '0' && character <= '9') ||
              (character == '-' && value.empty());
    if (valid && value.size() < 120) value += static_cast<char>(character);
    character = GetCharPressed();
  }
  if (IsKeyPressed(KEY_BACKSPACE) && !value.empty()) value.pop_back();
}

Rectangle Renderer::MapRectangle() const {
  if (screen_ == Screen::Results) {
    return {600, 160, static_cast<float>(screen_width_ - 624),
            static_cast<float>(screen_height_ - 238)};
  }
  if (screen_ == Screen::Summary) {
    return {24, 150, static_cast<float>(screen_width_ - 48),
            static_cast<float>(screen_height_ - 238)};
  }
  return {24, 82, static_cast<float>(screen_width_ - 48),
          static_cast<float>(screen_height_ - 158)};
}

Vector2 Renderer::WorldToScreen(Vector2 point) const {
  Rectangle map = MapRectangle();
  float range = static_cast<float>(world_max_ - world_min_);
  float x =
      map.x + (point.x - static_cast<float>(world_min_)) / range * map.width;
  float y =
      map.y +
      (1.0f - (point.y - static_cast<float>(world_min_)) / range) * map.height;
  return {map.x + (x - map.x) * zoom_ + pan_.x,
          map.y + (y - map.y) * zoom_ + pan_.y};
}

Vector2 Renderer::ScreenToWorld(Vector2 point) const {
  Rectangle map = MapRectangle();
  float x = map.x + (point.x - map.x - pan_.x) / zoom_;
  float y = map.y + (point.y - map.y - pan_.y) / zoom_;
  float range = static_cast<float>(world_max_ - world_min_);
  return {static_cast<float>(world_min_) + (x - map.x) / map.width * range,
          static_cast<float>(world_min_) +
              (1.0f - (y - map.y) / map.height) * range};
}

bool Renderer::InMap(Vector2 point) const {
  return CheckCollisionPointRec(point, MapRectangle());
}

Color Renderer::ClusterColor(int index) const {
  static const Color colors[] = {{82, 178, 255, 255},  {255, 112, 126, 255},
                                 {91, 214, 151, 255},  {249, 190, 75, 255},
                                 {185, 123, 255, 255}, {61, 208, 203, 255},
                                 {255, 137, 214, 255}, {177, 216, 77, 255}};
  if (index < 0) return {121, 135, 158, 255};
  return colors[index % (sizeof(colors) / sizeof(colors[0]))];
}

void Renderer::SetSeed(int seed) {
  selected_seed_ = seed;
  seed_input_ = std::to_string(seed);
}

void Renderer::SetEmptyCentroidNotifications(const std::vector<int>& indices,
                                             int iteration) {
  empty_centroids_ = indices;
  empty_iteration_ = iteration;
}

void Renderer::SetMultiRunResults(const std::vector<double>& costs,
                                  const std::vector<Point>& points,
                                  const std::vector<Centroid>& centroids,
                                  int best_run) {
  result_costs_ = costs;
  result_points_ = points;
  result_centroids_ = centroids;
  best_run_ = best_run;
  result_scroll_ = 0;
  screen_ = Screen::Results;
}

void Renderer::ClearMultiRunResults() {
  result_costs_.clear();
  result_points_.clear();
  result_centroids_.clear();
  best_run_ = -1;
  if (screen_ == Screen::Results) screen_ = Screen::Simulation;
}

void Renderer::BeginHardReset(int k, CentroidInitialization method) {
  selected_k_ = std::clamp(k, kMinK, kMaxK);
  selected_method_ = method;
  manual_centroids_.clear();
  hard_reset_ = true;
  screen_ = Screen::Setup;
}

void Renderer::ShowMainMenu() {
  ClearMultiRunResults();
  manual_centroids_.clear();
  hard_reset_ = false;
  screen_ = Screen::Setup;
  ResetView();
}

void Renderer::ResetView() {
  zoom_ = 1.0f;
  pan_ = {0.0f, 0.0f};
}
