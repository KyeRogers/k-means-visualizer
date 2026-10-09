#include "renderer.hpp"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>

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
    {"Unassigned", {130, 130, 130, 255}}, {"Red", {220, 70, 70, 255}},
    {"Blue", {70, 120, 220, 255}},        {"Green", {70, 170, 90, 255}},
    {"Orange", {230, 145, 55, 255}},      {"Purple", {150, 85, 190, 255}},
    {"Cyan", {50, 170, 180, 255}},        {"Pink", {220, 90, 150, 255}},
    {"Yellow", {220, 190, 55, 255}},      {"Brown", {155, 105, 70, 255}},
    {"Lime", {125, 180, 65, 255}},        {"Magenta", {190, 70, 170, 255}},
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
  SetExitKey(KEY_Q);
  SetTargetFPS(60);

  filename_input_.clear();
  filename_active_ = false;
  show_help_ = false;
  show_setup_ = true;

  selected_k_ = 2;
  seed_input_active_ = false;
  seed_input_ = std::to_string(selected_seed_);
  empty_centroid_indices_.clear();
  empty_centroid_iteration_ = -1;
  selected_runs_ = kDefaultRuns;
  show_multi_run_setup_ = false;
  show_multi_run_results_ = false;
  show_manual_setup_ = false;
  show_converged_map_ = false;
  hard_reset_setup_ = false;
  selected_method_ = CentroidInitialization::Randomized;
  manual_centroids_.clear();
  multi_run_costs_.clear();
  multi_run_points_.clear();
  multi_run_centroids_.clear();
  multi_run_best_run_ = -1;
  multi_run_cost_scroll_ = 0;

  zoom_ = 1.0f;
  pan_ = {0.0f, 0.0f};
}

bool Renderer::ShouldClose() const { return WindowShouldClose(); }

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
      if (filename_active_ && character >= 32 && character <= 126) {
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
      const float panel_width =
          std::min(680.0f, static_cast<float>(screen_width_) - 40.0f);
      const float panel_height =
          std::min(590.0f, static_cast<float>(screen_height_) - 30.0f);
      const float panel_x = (screen_width_ - panel_width) / 2.0f;
      const float panel_y = (screen_height_ - panel_height) / 2.0f;
      // Keep the hit target aligned with the filename field drawn by
      // DrawHelp(). DrawHelp places the field after the 12 help rows and the
      // section heading.
      const float filename_y =
          panel_y + 58.0f + 12.0f * 23.0f + 7.0f + 13.0f + 25.0f;
      Rectangle filename_box = {panel_x + 22.0f, filename_y,
                                panel_width - 44.0f, 36.0f};
      filename_active_ = CheckCollisionPointRec(mouse, filename_box);
    }

    return events;
  }

  // ------------------------------------------------------------
  // MULTI-RUN RESULTS SCREEN
  // ------------------------------------------------------------

  if (show_multi_run_results_) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_M)) {
      events.main_menu = true;
      ClearMultiRunResults();
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f && !multi_run_costs_.empty()) {
      Vector2 mouse = GetMousePosition();
      int cost_panel_x = static_cast<int>(screen_width_ * 0.67f);

      if (mouse.x >= cost_panel_x && mouse.y >= kTopBarHeight) {
        const int rows =
            std::max(1, (screen_height_ - kTopBarHeight - 85) / 24);
        int max_scroll =
            std::max(0, static_cast<int>(multi_run_costs_.size()) - rows);

        if (wheel < 0.0f) {
          multi_run_cost_scroll_ =
              std::min(max_scroll, multi_run_cost_scroll_ + rows / 2);
        } else {
          multi_run_cost_scroll_ =
              std::max(0, multi_run_cost_scroll_ - rows / 2);
        }
      }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 mouse = GetMousePosition();

      Rectangle menu_button = {20.0f, screen_height_ - kBottomBarHeight + 11.0f,
                               150.0f, 36.0f};

      if (CheckCollisionPointRec(mouse, menu_button)) {
        events.main_menu = true;
        ClearMultiRunResults();
        show_setup_ = true;
      }
    }

    return events;
  }

  // ------------------------------------------------------------
  // MULTI-RUN SETUP
  // ------------------------------------------------------------

  if (show_multi_run_setup_) {
    if (IsKeyPressed(KEY_M)) {
      events.main_menu = true;
      return events;
    }

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
      selected_runs_ = std::max(kMinRuns, selected_runs_ - 1);
    }

    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
      selected_runs_ = std::min(kMaxRuns, selected_runs_ + 1);
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
      events.multi_run_requested = true;
      events.total_runs = selected_runs_;
      show_multi_run_setup_ = false;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
      show_multi_run_setup_ = false;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 mouse = GetMousePosition();

      Rectangle menu_button = {20.0f, screen_height_ - kBottomBarHeight + 11.0f,
                               150.0f, 36.0f};
      Rectangle minus_button = {screen_width_ / 2.0f - 120.0f,
                                screen_height_ / 2.0f - 20.0f, 50.0f, 40.0f};

      Rectangle plus_button = {screen_width_ / 2.0f + 70.0f,
                               screen_height_ / 2.0f - 20.0f, 50.0f, 40.0f};

      Rectangle start_button = {screen_width_ / 2.0f - 100.0f,
                                screen_height_ / 2.0f + 55.0f, 200.0f, 42.0f};

      Rectangle cancel_button = {screen_width_ / 2.0f - 100.0f,
                                 screen_height_ / 2.0f + 110.0f, 200.0f, 38.0f};

      if (CheckCollisionPointRec(mouse, menu_button)) {
        events.main_menu = true;
      } else if (CheckCollisionPointRec(mouse, minus_button)) {
        selected_runs_ = std::max(kMinRuns, selected_runs_ - 1);
      } else if (CheckCollisionPointRec(mouse, plus_button)) {
        selected_runs_ = std::min(kMaxRuns, selected_runs_ + 1);
      } else if (CheckCollisionPointRec(mouse, start_button)) {
        events.multi_run_requested = true;
        events.total_runs = selected_runs_;
        show_multi_run_setup_ = false;
      } else if (CheckCollisionPointRec(mouse, cancel_button)) {
        show_multi_run_setup_ = false;
      }
    }

    return events;
  }

  // ------------------------------------------------------------
  // MANUAL CENTROID SETUP
  // ------------------------------------------------------------

  if (show_manual_setup_) {
    if (IsKeyPressed(KEY_M)) {
      events.main_menu = true;
      return events;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
      show_manual_setup_ = false;
      manual_centroids_.clear();
      show_setup_ = true;
      return events;
    }

    if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_BACKSPACE)) {
      if (!manual_centroids_.empty()) {
        manual_centroids_.pop_back();
      }
    }

    if (IsKeyPressed(KEY_R)) {
      manual_centroids_.clear();
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) &&
        static_cast<int>(manual_centroids_.size()) < selected_k_) {
      Vector2 mouse = GetMousePosition();
      Rectangle manual_plot = {
          static_cast<float>(kLeftPanelWidth), 70.0f,
          static_cast<float>(screen_width_ - kLeftPanelWidth),
          static_cast<float>(screen_height_ - 70 - kBottomBarHeight)};

      if (IsPointInsidePlot(mouse, manual_plot)) {
        manual_centroids_.push_back(ScreenToWorld(mouse, manual_plot));
      }
    }

    bool ready = static_cast<int>(manual_centroids_.size()) == selected_k_;

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
      if (ready) {
        events.k_selected = true;
        events.k = selected_k_;
        events.initialization_method = CentroidInitialization::Manual;
        events.manual_centroids = manual_centroids_;
        show_manual_setup_ = false;
        hard_reset_setup_ = false;
      }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 mouse = GetMousePosition();
      Rectangle top_menu_button = {static_cast<float>(screen_width_) - 165.0f,
                                   18.0f, 145.0f, 34.0f};
      Rectangle start_button = {
          static_cast<float>(screen_width_) / 2.0f - 100.0f,
          static_cast<float>(screen_height_) - 100.0f, 200.0f, 42.0f};

      Rectangle back_button = {
          20.0f, static_cast<float>(screen_height_) - 52.0f, 120.0f, 36.0f};
      Rectangle footer_menu_button = {
          155.0f, static_cast<float>(screen_height_) - 52.0f, 150.0f, 36.0f};

      if (CheckCollisionPointRec(mouse, start_button) && ready) {
        events.k_selected = true;
        events.k = selected_k_;
        events.initialization_method = CentroidInitialization::Manual;
        events.manual_centroids = manual_centroids_;
        show_manual_setup_ = false;
        hard_reset_setup_ = false;
      } else if (CheckCollisionPointRec(mouse, top_menu_button) ||
                 CheckCollisionPointRec(mouse, footer_menu_button) ||
                 CheckCollisionPointRec(mouse, back_button)) {
        if (CheckCollisionPointRec(mouse, top_menu_button) ||
            CheckCollisionPointRec(mouse, footer_menu_button)) {
          events.main_menu = true;
          return events;
        }
        show_manual_setup_ = false;
        manual_centroids_.clear();
        show_setup_ = true;
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
      show_converged_map_ = false;
    }

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_M)) {
      events.main_menu = true;
      show_converged_map_ = false;
    }

    if (IsKeyPressed(KEY_V)) {
      show_converged_map_ = !show_converged_map_;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 mouse = GetMousePosition();

      if (show_converged_map_) {
        const float footer_y =
            static_cast<float>(screen_height_ - kBottomBarHeight + 11);
        Rectangle summary_button = {20.0f, footer_y, 140.0f, 36.0f};
        Rectangle reset_button = {170.0f, footer_y, 100.0f, 36.0f};
        Rectangle menu_button = {280.0f, footer_y, 150.0f, 36.0f};
        Rectangle hard_reset_button = {440.0f, footer_y, 150.0f, 36.0f};

        if (CheckCollisionPointRec(mouse, summary_button)) {
          show_converged_map_ = false;
        } else if (CheckCollisionPointRec(mouse, reset_button)) {
          events.reset = true;
          show_converged_map_ = false;
        } else if (CheckCollisionPointRec(mouse, menu_button)) {
          events.main_menu = true;
          show_converged_map_ = false;
        } else if (CheckCollisionPointRec(mouse, hard_reset_button)) {
          events.hard_reset_requested = true;
          show_converged_map_ = false;
        }
      } else {
        const float button_y = screen_height_ / 2.0f + 90.0f;
        Rectangle map_button = {screen_width_ / 2.0f - 270.0f, button_y, 160.0f,
                                45.0f};
        Rectangle reset_button = {screen_width_ / 2.0f - 80.0f, button_y,
                                  160.0f, 45.0f};
        Rectangle menu_button = {screen_width_ / 2.0f + 110.0f, button_y,
                                 160.0f, 45.0f};
        Rectangle hard_reset_button = {screen_width_ / 2.0f + 300.0f, button_y,
                                       160.0f, 45.0f};

        if (CheckCollisionPointRec(mouse, map_button)) {
          show_converged_map_ = true;
        } else if (CheckCollisionPointRec(mouse, reset_button)) {
          events.reset = true;
        } else if (CheckCollisionPointRec(mouse, menu_button)) {
          events.main_menu = true;
        } else if (CheckCollisionPointRec(mouse, hard_reset_button)) {
          events.hard_reset_requested = true;
        }
      }
    }

    if (show_converged_map_) {
      if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector2 delta = GetMouseDelta();
        pan_.x += delta.x;
        pan_.y += delta.y;
      }

      float wheel = GetMouseWheelMove();
      if (wheel != 0.0f) {
        Vector2 mouse = GetMousePosition();
        Vector2 before_zoom = ScreenToWorld(mouse);
        zoom_ = ClampFloat(zoom_ * (wheel > 0.0f ? 1.1f : 1.0f / 1.1f), 0.1f,
                           20.0f);
        Vector2 after_zoom = WorldToScreen(before_zoom);
        pan_.x += mouse.x - after_zoom.x;
        pan_.y += mouse.y - after_zoom.y;
      }
    }

    return events;
  }

  // ------------------------------------------------------------
  // SETUP SCREEN
  // ------------------------------------------------------------

  if (show_setup_) {
    Rectangle menu_button = {static_cast<float>(screen_width_) - 165.0f, 13.0f,
                             145.0f, 34.0f};
    if (hard_reset_setup_ && IsKeyPressed(KEY_ESCAPE)) {
      show_setup_ = false;
      hard_reset_setup_ = false;
      return events;
    }
    if (IsKeyPressed(KEY_M) && !hard_reset_setup_) {
      events.main_menu = true;
      return events;
    }

    // Seed input is deliberately committed on Enter (or when the user
    // clicks Randomize), so editing the field never changes a running run.
    bool seed_commit_pressed = false;
    if (seed_input_active_) {
      int character = GetCharPressed();
      while (character > 0) {
        if ((character >= '0' && character <= '9') ||
            (character == '-' && seed_input_.empty())) {
          if (seed_input_.size() < 11)
            seed_input_ += static_cast<char>(character);
        }
        character = GetCharPressed();
      }
      if (IsKeyPressed(KEY_BACKSPACE) && !seed_input_.empty()) {
        seed_input_.pop_back();
      }
      if (IsKeyPressed(KEY_ENTER)) {
        seed_commit_pressed = true;
        char* end = nullptr;
        long parsed = std::strtol(seed_input_.c_str(), &end, 10);
        if (end != seed_input_.c_str() && *end == '\0' && parsed >= INT_MIN &&
            parsed <= INT_MAX) {
          selected_seed_ = static_cast<int>(parsed);
          events.seed_changed = true;
          events.seed = selected_seed_;
        }
        seed_input_ = std::to_string(selected_seed_);
        seed_input_active_ = false;
      }
    }

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
      seed_input_active_ = false;
      selected_k_ = std::max(kMinK, selected_k_ - 1);
    }

    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
      seed_input_active_ = false;
      selected_k_ = std::min(kMaxK, selected_k_ + 1);
    }

    if (IsKeyPressed(KEY_ONE)) {
      selected_method_ = CentroidInitialization::Randomized;
    }
    if (IsKeyPressed(KEY_TWO)) {
      selected_method_ = CentroidInitialization::KMeansPlusPlus;
    }
    if (IsKeyPressed(KEY_THREE)) {
      selected_method_ = CentroidInitialization::Manual;
    }

    auto begin_selection = [&]() {
      if (selected_method_ == CentroidInitialization::Manual) {
        manual_centroids_.clear();
        show_manual_setup_ = true;
        show_setup_ = false;
        return;
      }

      events.k_selected = true;
      events.k = selected_k_;
      events.initialization_method = selected_method_;
      events.manual_centroids.clear();
      show_setup_ = false;
      hard_reset_setup_ = false;
    };

    if (!seed_commit_pressed &&
        (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))) {
      begin_selection();
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 mouse = GetMousePosition();

      if (CheckCollisionPointRec(mouse, menu_button)) {
        if (hard_reset_setup_) {
          show_setup_ = false;
          hard_reset_setup_ = false;
        } else {
          events.main_menu = true;
        }
        return events;
      }

      Rectangle seed_box = {screen_width_ / 2.0f - 170.0f, 244.0f, 220.0f,
                            38.0f};
      Rectangle random_seed_button = {screen_width_ / 2.0f + 60.0f, 244.0f,
                                      150.0f, 38.0f};
      if (CheckCollisionPointRec(mouse, seed_box)) {
        seed_input_active_ = true;
      } else if (CheckCollisionPointRec(mouse, random_seed_button)) {
        selected_seed_ = GetRandomValue(-1000000000, 1000000000);
        seed_input_ = std::to_string(selected_seed_);
        seed_input_active_ = false;
        events.seed_changed = true;
        events.seed = selected_seed_;
      } else {
        seed_input_active_ = false;
      }

      Rectangle minus_button = {screen_width_ / 2.0f - 180.0f, 180.0f, 50.0f,
                                42.0f};

      Rectangle plus_button = {screen_width_ / 2.0f + 130.0f, 180.0f, 50.0f,
                               42.0f};

      const float card_width = 250.0f;
      const float card_height = 150.0f;
      const float card_gap = 20.0f;
      const float cards_width = card_width * 3.0f + card_gap * 2.0f;
      const float cards_x = (screen_width_ - cards_width) / 2.0f;
      const float cards_y = 300.0f;

      Rectangle random_card = {cards_x, cards_y, card_width, card_height};
      Rectangle plus_card = {cards_x + card_width + card_gap, cards_y,
                             card_width, card_height};
      Rectangle manual_card = {cards_x + 2.0f * (card_width + card_gap),
                               cards_y, card_width, card_height};

      Rectangle start_button = {screen_width_ / 2.0f - 100.0f, 485.0f, 200.0f,
                                44.0f};

      if (CheckCollisionPointRec(mouse, menu_button)) {
        events.main_menu = true;
      } else if (hard_reset_setup_ &&
                 CheckCollisionPointRec(
                     mouse, {screen_width_ - 165.0f, screen_height_ - 55.0f,
                             145.0f, 36.0f})) {
        show_setup_ = false;
        hard_reset_setup_ = false;
      } else if (CheckCollisionPointRec(mouse, minus_button)) {
        selected_k_ = std::max(kMinK, selected_k_ - 1);
      } else if (CheckCollisionPointRec(mouse, plus_button)) {
        selected_k_ = std::min(kMaxK, selected_k_ + 1);
      } else if (CheckCollisionPointRec(mouse, random_card)) {
        selected_method_ = CentroidInitialization::Randomized;
      } else if (CheckCollisionPointRec(mouse, plus_card)) {
        selected_method_ = CentroidInitialization::KMeansPlusPlus;
      } else if (CheckCollisionPointRec(mouse, manual_card)) {
        selected_method_ = CentroidInitialization::Manual;
      } else if (CheckCollisionPointRec(mouse, start_button)) {
        begin_selection();
      }
    }

    return events;
  }

  // ------------------------------------------------------------
  // MULTI-RUN TRIGGER
  // ------------------------------------------------------------

  if (IsKeyPressed(KEY_M)) {
    show_multi_run_setup_ = true;
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

  if (IsKeyPressed(KEY_LEFT_BRACKET)) {
    events.speed_change = -1;
  } else if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
    events.speed_change = 1;
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
        15.0f, static_cast<float>(screen_height_) - kBottomBarHeight + 12.0f,
        90.0f, 34.0f};

    Rectangle run_button = {
        115.0f, static_cast<float>(screen_height_) - kBottomBarHeight + 12.0f,
        100.0f, 34.0f};

    Rectangle reset_button = {
        225.0f, static_cast<float>(screen_height_) - kBottomBarHeight + 12.0f,
        90.0f, 34.0f};

    Rectangle multi_run_button = {
        325.0f, static_cast<float>(screen_height_) - kBottomBarHeight + 12.0f,
        120.0f, 34.0f};
    Rectangle menu_button = {static_cast<float>(screen_width_) - 165.0f, 13.0f,
                             145.0f, 34.0f};
    Rectangle hard_reset_button = {static_cast<float>(screen_width_) - 325.0f,
                                   13.0f, 145.0f, 34.0f};
    Rectangle slower_button = {
        470.0f, static_cast<float>(screen_height_) - kBottomBarHeight + 12.0f,
        36.0f, 34.0f};
    Rectangle faster_button = {
        512.0f, static_cast<float>(screen_height_) - kBottomBarHeight + 12.0f,
        36.0f, 34.0f};

    if (CheckCollisionPointRec(mouse, step_button)) {
      events.step = true;
    } else if (CheckCollisionPointRec(mouse, run_button)) {
      events.toggle_running = true;
    } else if (CheckCollisionPointRec(mouse, reset_button)) {
      events.reset = true;
    } else if (CheckCollisionPointRec(mouse, multi_run_button)) {
      show_multi_run_setup_ = true;
    } else if (CheckCollisionPointRec(mouse, slower_button)) {
      events.speed_change = -1;
    } else if (CheckCollisionPointRec(mouse, faster_button)) {
      events.speed_change = 1;
    } else if (CheckCollisionPointRec(mouse, menu_button)) {
      events.main_menu = true;
    } else if (CheckCollisionPointRec(mouse, hard_reset_button)) {
      events.hard_reset_requested = true;
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
                      const std::vector<Centroid>& centroids, double total_cost,
                      int iteration, bool converged,
                      double run_interval_seconds) {
  screen_width_ = GetScreenWidth();
  screen_height_ = GetScreenHeight();

  BeginDrawing();
  ClearBackground(kBackground);

  // ------------------------------------------------------------
  // MULTI-RUN RESULTS SCREEN
  // ------------------------------------------------------------

  if (show_multi_run_results_) {
    DrawMultiRunResults();
    EndDrawing();
    return;
  }

  // ------------------------------------------------------------
  // MULTI-RUN SETUP SCREEN
  // ------------------------------------------------------------

  if (show_multi_run_setup_) {
    DrawMultiRunSetup();
    EndDrawing();
    return;
  }

  // ------------------------------------------------------------
  // MANUAL CENTROID SETUP SCREEN
  // ------------------------------------------------------------

  if (show_manual_setup_) {
    DrawManualSetup(points);
    EndDrawing();
    return;
  }

  // ------------------------------------------------------------
  // CONVERGED SCREEN
  // ------------------------------------------------------------

  if (converged) {
    DrawConvergedScreen(points, centroids, total_cost, iteration);

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

  DrawTopBar(static_cast<int>(centroids.size()), iteration, total_cost,
             selected_seed_);

  DrawRectangle(0, kTopBarHeight, kLeftPanelWidth,
                screen_height_ - kTopBarHeight - kBottomBarHeight, kPanel);

  DrawRectangle(kLeftPanelWidth, kTopBarHeight, screen_width_ - kLeftPanelWidth,
                screen_height_ - kTopBarHeight - kBottomBarHeight, kWhite);

  DrawSidePanel(static_cast<int>(centroids.size()));

  Rectangle plot_clip = {
      static_cast<float>(kLeftPanelWidth), static_cast<float>(kTopBarHeight),
      static_cast<float>(screen_width_ - kLeftPanelWidth),
      static_cast<float>(screen_height_ - kTopBarHeight - kBottomBarHeight)};
  DrawPoints(points, plot_clip);
  DrawCentroids(centroids, plot_clip);

  DrawBottomBar(run_interval_seconds);
  DrawEmptyCentroidNotice();

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

void Renderer::DrawTopBar(int k, int iteration, double total_cost,
                          int seed) const {
  DrawRectangle(0, 0, screen_width_, kTopBarHeight, kPanelDark);

  DrawText("K-MEANS", 18, 17, 24, kText);

  char buffer[128];

  std::snprintf(buffer, sizeof(buffer), "K: %d", k);

  DrawText(buffer, 220, 20, 20, kText);

  std::snprintf(buffer, sizeof(buffer), "Iteration: %d", iteration);

  DrawText(buffer, 290, 20, 20, kText);

  std::snprintf(buffer, sizeof(buffer), "Cost: %.4f", total_cost);

  DrawText(buffer, 470, 20, 20, kText);

  std::snprintf(buffer, sizeof(buffer), "Seed: %d", seed);
  DrawText(buffer, 650, 20, 18, kText);

  DrawText("F1: Help", screen_width_ - 245, 21, 16, kMutedText);
  DrawButton({static_cast<float>(screen_width_) - 325.0f, 13.0f, 145.0f, 34.0f},
             "HARD RESET");
  DrawButton({static_cast<float>(screen_width_) - 165.0f, 13.0f, 145.0f, 34.0f},
             "MAIN MENU");
}

void Renderer::DrawBottomBar(double run_interval_seconds) const {
  int y = screen_height_ - kBottomBarHeight;

  DrawRectangle(0, y, screen_width_, kBottomBarHeight, kPanelDark);

  DrawButton({15.0f, static_cast<float>(y) + 12.0f, 90.0f, kButtonHeight},
             "STEP");

  DrawButton({115.0f, static_cast<float>(y) + 12.0f, 100.0f, kButtonHeight},
             "RUN / STOP");

  DrawButton({225.0f, static_cast<float>(y) + 12.0f, 90.0f, kButtonHeight},
             "SOFT RESET");

  DrawButton({325.0f, static_cast<float>(y) + 12.0f, 120.0f, kButtonHeight},
             "MULTI-RUN");

  DrawButton({470.0f, static_cast<float>(y) + 12.0f, 36.0f, kButtonHeight},
             "-");
  DrawButton({512.0f, static_cast<float>(y) + 12.0f, 36.0f, kButtonHeight},
             "+");

  char speed_text[32];
  std::snprintf(speed_text, sizeof(speed_text), "%.2g s/step",
                run_interval_seconds);
  DrawText(speed_text, 556, y + 20, 15, kText);

  DrawText("SPACE: Step", 665, y + 12, 14, kMutedText);
  DrawText("R: Run/Stop", 665, y + 31, 14, kMutedText);

  DrawText("ESC: Reset", 780, y + 12, 14, kMutedText);
  DrawText("M: Multi-run", 780, y + 31, 14, kMutedText);

  DrawText("Q: Quit", 900, y + 12, 14, kText);
  DrawText("F1: Help", 900, y + 31, 14, kMutedText);

  DrawText("Right click: add centroid in Manual mode", 985, y + 12, 14,
           kMutedText);
  DrawText("[ / ]: slower / faster", 985, y + 31, 14, kMutedText);
}

void Renderer::DrawSidePanel(int k) const {
  DrawText("CLUSTERS", 18, kTopBarHeight + 18, 18, kText);

  int y = kTopBarHeight + 55;

  for (int i = 0; i < k; ++i) {
    int color_index = i + 1;

    if (color_index >= kColorCount) {
      color_index = 1 + (i % (kColorCount - 1));
    }

    DrawCircle(28, y + 9, 7.0f, kPalette[color_index].color);

    char label[64];

    std::snprintf(label, sizeof(label), "Centroid %d", i);

    DrawText(label, 45, y, 16, kText);

    y += 28;
  }

  DrawLine(15, y + 5, kLeftPanelWidth - 15, y + 5, kBorder);

  DrawText("WORLD", 18, y + 25, 18, kText);

  char buffer[128];

  std::snprintf(buffer, sizeof(buffer), "X: %.1f - %.1f", world_min_,
                world_max_);

  DrawText(buffer, 18, y + 52, 14, kMutedText);

  std::snprintf(buffer, sizeof(buffer), "Y: %.1f - %.1f", world_min_,
                world_max_);

  DrawText(buffer, 18, y + 73, 14, kMutedText);

  std::snprintf(buffer, sizeof(buffer), "Zoom: %.2fx", zoom_);

  DrawText(buffer, 18, y + 105, 14, kMutedText);

  DrawText("Middle mouse: Pan", 18, y + 130, 13, kMutedText);

  DrawText("Wheel: Zoom", 18, y + 149, 13, kMutedText);
}

void Renderer::DrawPoints(const std::vector<Point>& points,
                          Rectangle clip) const {
  BeginScissorMode(static_cast<int>(clip.x), static_cast<int>(clip.y),
                   static_cast<int>(clip.width), static_cast<int>(clip.height));
  for (const Point& point : points) {
    Vector2 screen_point = WorldToScreen(
        {static_cast<float>(point.GetX()), static_cast<float>(point.GetY())},
        clip);

    DrawCircleV(screen_point, kPointRadius, GetPointColor(point.GetCentroid()));
  }
  EndScissorMode();
}

void Renderer::DrawCentroids(const std::vector<Centroid>& centroids,
                             Rectangle clip) const {
  BeginScissorMode(static_cast<int>(clip.x), static_cast<int>(clip.y),
                   static_cast<int>(clip.width), static_cast<int>(clip.height));
  for (std::size_t i = 0; i < centroids.size(); ++i) {
    Vector2 screen_point =
        WorldToScreen({static_cast<float>(centroids[i].GetX()),
                       static_cast<float>(centroids[i].GetY())},
                      clip);

    Color color = GetCentroidColor(static_cast<int>(i));

    DrawLineEx({screen_point.x - kCentroidSize, screen_point.y - kCentroidSize},
               {screen_point.x + kCentroidSize, screen_point.y + kCentroidSize},
               4.0f, color);

    DrawLineEx({screen_point.x - kCentroidSize, screen_point.y + kCentroidSize},
               {screen_point.x + kCentroidSize, screen_point.y - kCentroidSize},
               4.0f, color);
  }
  EndScissorMode();
}

Color Renderer::GetPointColor(int centroid) const {
  if (centroid <= 0) {
    return kUnassigned;
  }

  int color_index = centroid;

  if (color_index >= kColorCount) {
    color_index = 1 + ((color_index - 1) % (kColorCount - 1));
  }

  return kPalette[color_index].color;
}

Color Renderer::GetCentroidColor(int centroid_index) const {
  int color_index = centroid_index + 1;

  if (color_index >= kColorCount) {
    color_index = 1 + (centroid_index % (kColorCount - 1));
  }

  return kPalette[color_index].color;
}

Vector2 Renderer::WorldToScreen(Vector2 point) const {
  return WorldToScreen(
      point,
      {static_cast<float>(kLeftPanelWidth), static_cast<float>(kTopBarHeight),
       static_cast<float>(screen_width_ - kLeftPanelWidth),
       static_cast<float>(screen_height_ - kTopBarHeight - kBottomBarHeight)});
}

Vector2 Renderer::WorldToScreen(Vector2 point, Rectangle plot) const {
  float plot_left = plot.x;
  float plot_top = plot.y;
  float plot_width = plot.width;
  float plot_height = plot.height;

  double world_range = world_max_ - world_min_;

  if (world_range <= 0.0) {
    return {plot_left + plot_width / 2.0f, plot_top + plot_height / 2.0f};
  }

  float normalized_x = static_cast<float>(
      (static_cast<double>(point.x) - world_min_) / world_range);

  float normalized_y = static_cast<float>(
      (static_cast<double>(point.y) - world_min_) / world_range);

  float x = plot_left + normalized_x * plot_width;
  float y = plot_top + (1.0f - normalized_y) * plot_height;

  x = plot_left + (x - plot_left) * zoom_ + pan_.x;

  y = plot_top + (y - plot_top) * zoom_ + pan_.y;

  return {x, y};
}

Vector2 Renderer::ScreenToWorld(Vector2 point) const {
  return ScreenToWorld(
      point,
      {static_cast<float>(kLeftPanelWidth), static_cast<float>(kTopBarHeight),
       static_cast<float>(screen_width_ - kLeftPanelWidth),
       static_cast<float>(screen_height_ - kTopBarHeight - kBottomBarHeight)});
}

Vector2 Renderer::ScreenToWorld(Vector2 point, Rectangle plot) const {
  float plot_left = plot.x;
  float plot_top = plot.y;
  float plot_width = plot.width;
  float plot_height = plot.height;

  double world_range = world_max_ - world_min_;

  if (world_range <= 0.0 || plot_width <= 0.0f || plot_height <= 0.0f) {
    return {static_cast<float>(world_min_), static_cast<float>(world_min_)};
  }

  float unpanned_x = plot_left + (point.x - plot_left - pan_.x) / zoom_;

  float unpanned_y = plot_top + (point.y - plot_top - pan_.y) / zoom_;

  float normalized_x = (unpanned_x - plot_left) / plot_width;

  float normalized_y = 1.0f - (unpanned_y - plot_top) / plot_height;

  double world_x = world_min_ + static_cast<double>(normalized_x) * world_range;

  double world_y = world_min_ + static_cast<double>(normalized_y) * world_range;

  return {static_cast<float>(world_x), static_cast<float>(world_y)};
}

bool Renderer::IsPointInsidePlot(Vector2 point) const {
  return IsPointInsidePlot(
      point,
      {static_cast<float>(kLeftPanelWidth), static_cast<float>(kTopBarHeight),
       static_cast<float>(screen_width_ - kLeftPanelWidth),
       static_cast<float>(screen_height_ - kTopBarHeight - kBottomBarHeight)});
}

bool Renderer::IsPointInsidePlot(Vector2 point, Rectangle plot) const {
  return point.x >= plot.x && point.x <= plot.x + plot.width &&
         point.y >= plot.y && point.y <= plot.y + plot.height;
}

void Renderer::DrawButton(Rectangle rectangle, const char* text,
                          bool selected) const {
  Color background = selected ? kPanelDark : kWhite;

  DrawRectangleRec(rectangle, background);

  DrawRectangleLinesEx(rectangle, 1.0f, kBorder);

  int text_width = MeasureText(text, 16);

  DrawText(
      text,
      static_cast<int>(rectangle.x + (rectangle.width - text_width) / 2.0f),
      static_cast<int>(rectangle.y + (rectangle.height - 16) / 2.0f), 16,
      kText);
}

void Renderer::DrawSetup() const {
  DrawRectangle(0, 0, screen_width_, screen_height_, kBackground);

  const char* title =
      hard_reset_setup_ ? "HARD RESET: CHANGE K / MODE" : "K-MEANS SETUP";
  int title_width = MeasureText(title, 38);
  DrawText(title, (screen_width_ - title_width) / 2, 45, 38, kText);
  DrawButton({static_cast<float>(screen_width_) - 165.0f, 13.0f, 145.0f, 34.0f},
             hard_reset_setup_ ? "CANCEL" : "MAIN MENU");

  const char* subtitle =
      "Choose K and how the initial centroids should be selected";
  int subtitle_width = MeasureText(subtitle, 18);
  DrawText(subtitle, (screen_width_ - subtitle_width) / 2, 92, 18, kMutedText);

  DrawText("NUMBER OF CLUSTERS (K)", 40, 150, 18, kText);

  Rectangle minus_button = {screen_width_ / 2.0f - 180.0f, 180.0f, 50.0f,
                            42.0f};
  Rectangle plus_button = {screen_width_ / 2.0f + 130.0f, 180.0f, 50.0f, 42.0f};

  DrawButton(minus_button, "-");
  DrawButton(plus_button, "+");

  char k_text[32];
  std::snprintf(k_text, sizeof(k_text), "%d", selected_k_);
  int k_width = MeasureText(k_text, 34);
  DrawText(k_text, (screen_width_ - k_width) / 2, 181, 34, kText);

  DrawText("RANDOM SEED", static_cast<int>(screen_width_ / 2.0f - 170.0f), 220,
           14, kMutedText);
  Rectangle seed_box = {screen_width_ / 2.0f - 170.0f, 244.0f, 220.0f, 38.0f};
  DrawRectangleRec(seed_box, kWhite);
  DrawRectangleLinesEx(seed_box, 1.0f, seed_input_active_ ? kText : kBorder);
  const char* seed_text =
      seed_input_.empty() ? "Type integer seed" : seed_input_.c_str();
  DrawText(seed_text, static_cast<int>(seed_box.x + 10),
           static_cast<int>(seed_box.y + 10), 16,
           seed_input_.empty() ? kMutedText : kText);
  DrawButton({screen_width_ / 2.0f + 60.0f, 244.0f, 150.0f, 38.0f},
             "RANDOMIZE SEED");

  const float card_width = 250.0f;
  const float card_height = 150.0f;
  const float card_gap = 20.0f;
  const float cards_width = card_width * 3.0f + card_gap * 2.0f;
  const float cards_x = (screen_width_ - cards_width) / 2.0f;
  const float cards_y = 300.0f;

  Rectangle cards[3] = {
      {cards_x, cards_y, card_width, card_height},
      {cards_x + card_width + card_gap, cards_y, card_width, card_height},
      {cards_x + 2.0f * (card_width + card_gap), cards_y, card_width,
       card_height}};

  const char* titles[3] = {"RANDOMIZED", "K-MEANS++", "MANUAL"};

  const char* descriptions[3] = {"Choose initial centroids\nrandomly.",
                                 "Uses placed points\non the first step.",
                                 "Place each centroid\nwith right click."};

  const char* shortcuts[3] = {"1", "2", "3"};

  for (int i = 0; i < 3; ++i) {
    CentroidInitialization method = i == 0 ? CentroidInitialization::Randomized
                                    : i == 1
                                        ? CentroidInitialization::KMeansPlusPlus
                                        : CentroidInitialization::Manual;

    bool selected = selected_method_ == method;

    DrawButton(cards[i], titles[i], selected);

    if (selected) {
      DrawRectangleLinesEx(cards[i], 3.0f, kText);
    }

    DrawText(shortcuts[i], static_cast<int>(cards[i].x + 12),
             static_cast<int>(cards[i].y + 12), 14, kMutedText);

    int title_width_i = MeasureText(titles[i], 20);
    DrawText(
        titles[i],
        static_cast<int>(cards[i].x + (cards[i].width - title_width_i) / 2),
        static_cast<int>(cards[i].y + 42), 20, kText);

    const char* line1 = descriptions[i];
    const char* newline = std::strchr(line1, '\n');

    if (newline != nullptr) {
      std::string first(line1, newline);
      std::string second(newline + 1);
      int w1 = MeasureText(first.c_str(), 15);
      int w2 = MeasureText(second.c_str(), 15);
      DrawText(first.c_str(),
               static_cast<int>(cards[i].x + (cards[i].width - w1) / 2),
               static_cast<int>(cards[i].y + 78), 15, kMutedText);
      DrawText(second.c_str(),
               static_cast<int>(cards[i].x + (cards[i].width - w2) / 2),
               static_cast<int>(cards[i].y + 99), 15, kMutedText);
    }
  }

  Rectangle start_button = {screen_width_ / 2.0f - 100.0f, 485.0f, 200.0f,
                            44.0f};

  DrawButton(start_button, selected_method_ == CentroidInitialization::Manual
                               ? "PLACE CENTROIDS"
                               : "START");

  // Keep footer instructions away from the quit label. The hard-reset cancel
  // action is in the top-right button, so it cannot overlap the footer either.
  DrawText("Seed: click to edit; Enter applies.  1/2/3: mode; Enter: continue",
           40, screen_height_ - 48, 14, kMutedText);
  DrawText("Q: Quit", screen_width_ - 90, screen_height_ - 48, 16, kText);
}

void Renderer::DrawManualSetup(const std::vector<Point>& points) const {
  DrawRectangle(0, 0, screen_width_, screen_height_, kBackground);

  DrawRectangle(0, 0, screen_width_, 70, kPanelDark);
  DrawButton({static_cast<float>(screen_width_) - 165.0f, 18.0f, 145.0f, 34.0f},
             "MAIN MENU");

  char title[128];
  std::snprintf(title, sizeof(title), "MANUAL CENTROIDS    %d / %d PLACED",
                static_cast<int>(manual_centroids_.size()), selected_k_);

  DrawText(title, 22, 18, 26, kText);
  DrawText("Right click on the map to place a centroid", 22, 46, 14,
           kMutedText);

  DrawRectangle(0, 70, kLeftPanelWidth, screen_height_ - 70, kPanel);
  DrawRectangle(kLeftPanelWidth, 70, screen_width_ - kLeftPanelWidth,
                screen_height_ - 70, kWhite);

  DrawText("INSTRUCTIONS", 18, 95, 18, kText);

  DrawText("Right click", 18, 135, 15, kText);
  DrawText("Place centroid", 18, 157, 14, kMutedText);

  DrawText("Z / Backspace", 18, 195, 15, kText);
  DrawText("Undo last", 18, 217, 14, kMutedText);

  DrawText("R", 18, 255, 15, kText);
  DrawText("Clear all", 18, 277, 14, kMutedText);

  DrawText("ESC", 18, 315, 15, kText);
  DrawText("Back to setup", 18, 337, 14, kMutedText);

  DrawText("Q", 18, 375, 15, kText);
  DrawText("Quit application", 18, 397, 14, kMutedText);

  // Existing data points.
  Rectangle manual_plot = {
      static_cast<float>(kLeftPanelWidth), 70.0f,
      static_cast<float>(screen_width_ - kLeftPanelWidth),
      static_cast<float>(screen_height_ - 70 - kBottomBarHeight)};
  DrawPoints(points, manual_plot);

  // Manual centroids are rendered as numbered crosses.
  BeginScissorMode(static_cast<int>(manual_plot.x),
                   static_cast<int>(manual_plot.y),
                   static_cast<int>(manual_plot.width),
                   static_cast<int>(manual_plot.height));
  for (std::size_t i = 0; i < manual_centroids_.size(); ++i) {
    Vector2 screen_point = WorldToScreen(manual_centroids_[i], manual_plot);
    Color color = GetCentroidColor(static_cast<int>(i));

    DrawCircleLinesV(screen_point, 15.0f, color);
    DrawLineEx({screen_point.x - 10.0f, screen_point.y},
               {screen_point.x + 10.0f, screen_point.y}, 4.0f, color);
    DrawLineEx({screen_point.x, screen_point.y - 10.0f},
               {screen_point.x, screen_point.y + 10.0f}, 4.0f, color);

    char number[16];
    std::snprintf(number, sizeof(number), "%d", static_cast<int>(i + 1));
    DrawText(number, static_cast<int>(screen_point.x + 18),
             static_cast<int>(screen_point.y - 9), 15, color);
  }
  EndScissorMode();

  int footer_y = screen_height_ - 58;
  DrawRectangle(0, footer_y, screen_width_, 58, kPanelDark);

  bool ready = static_cast<int>(manual_centroids_.size()) == selected_k_;

  Rectangle back_button = {20.0f, footer_y + 11.0f, 120.0f, 36.0f};
  Rectangle menu_button = {155.0f, footer_y + 11.0f, 150.0f, 36.0f};
  Rectangle start_button = {screen_width_ / 2.0f - 100.0f, footer_y + 8.0f,
                            200.0f, 42.0f};

  DrawButton(back_button, "BACK");
  DrawButton(menu_button, "MAIN MENU");
  DrawButton(start_button, ready ? "START" : "PLACE ALL", ready);

  DrawText("Q: Quit", screen_width_ - 80, footer_y + 20, 14, kText);
}

void Renderer::SetSeed(int seed) {
  selected_seed_ = seed;
  seed_input_ = std::to_string(seed);
}

void Renderer::SetEmptyCentroidNotifications(const std::vector<int>& indices,
                                             int iteration) {
  empty_centroid_indices_ = indices;
  empty_centroid_iteration_ = indices.empty() ? -1 : iteration;
}

void Renderer::DrawEmptyCentroidNotice() const {
  if (empty_centroid_indices_.empty()) return;
  char message[256];
  std::string list;
  for (std::size_t i = 0; i < empty_centroid_indices_.size(); ++i) {
    if (i) list += ", ";
    list += std::to_string(empty_centroid_indices_[i]);
  }
  std::snprintf(message, sizeof(message), "Empty centroids reinitialized: %s",
                list.c_str());
  int width = MeasureText(message, 16) + 28;
  Rectangle box = {
      static_cast<float>(kLeftPanelWidth + 16),
      static_cast<float>(kTopBarHeight + 14),
      static_cast<float>(std::min(width, screen_width_ - kLeftPanelWidth - 32)),
      36.0f};
  DrawRectangleRec(box, {255, 247, 220, 245});
  DrawRectangleLinesEx(box, 1.0f, {220, 180, 90, 255});
  DrawText(message, static_cast<int>(box.x + 12), static_cast<int>(box.y + 10),
           16, kText);
}

void Renderer::SetMultiRunResults(const std::vector<double>& costs,
                                  const std::vector<Point>& best_points,
                                  const std::vector<Centroid>& best_centroids,
                                  int best_run) {
  multi_run_costs_ = costs;
  multi_run_points_ = best_points;
  multi_run_centroids_ = best_centroids;
  multi_run_best_run_ = best_run;
  multi_run_cost_scroll_ = 0;

  show_multi_run_setup_ = false;
  show_multi_run_results_ = true;

  screen_width_ = GetScreenWidth();
  screen_height_ = GetScreenHeight();
  zoom_ = 1.0f;
  pan_ = {0.0f, 0.0f};
}

void Renderer::ClearMultiRunResults() {
  show_multi_run_results_ = false;
  show_multi_run_setup_ = false;
  multi_run_costs_.clear();
  multi_run_points_.clear();
  multi_run_centroids_.clear();
  multi_run_best_run_ = -1;
  multi_run_cost_scroll_ = 0;
}

void Renderer::BeginHardReset(int k, CentroidInitialization method) {
  ClearMultiRunResults();
  selected_k_ = std::max(kMinK, std::min(kMaxK, k));
  selected_method_ = method;
  manual_centroids_.clear();
  show_setup_ = true;
  show_manual_setup_ = false;
  show_converged_map_ = false;
  hard_reset_setup_ = true;
}

void Renderer::ShowMainMenu() {
  ClearMultiRunResults();
  show_setup_ = true;
  show_manual_setup_ = false;
  show_converged_map_ = false;
  hard_reset_setup_ = false;
  show_help_ = false;
  filename_active_ = false;
  selected_method_ = CentroidInitialization::Randomized;
  selected_k_ = 2;
  manual_centroids_.clear();
  zoom_ = 1.0f;
  pan_ = {0.0f, 0.0f};
}

void Renderer::DrawMultiRunSetup() const {
  DrawRectangle(0, 0, screen_width_, screen_height_, kBackground);

  const char* title = "MULTI-RUN K-MEANS";
  int title_width = MeasureText(title, 40);
  DrawText(title, (screen_width_ - title_width) / 2, screen_height_ / 2 - 155,
           40, kText);

  const char* subtitle = "Select how many independent runs to perform";
  int subtitle_width = MeasureText(subtitle, 20);
  DrawText(subtitle, (screen_width_ - subtitle_width) / 2,
           screen_height_ / 2 - 95, 20, kMutedText);

  Rectangle minus_button = {screen_width_ / 2.0f - 120.0f,
                            screen_height_ / 2.0f - 20.0f, 50.0f, 40.0f};

  Rectangle plus_button = {screen_width_ / 2.0f + 70.0f,
                           screen_height_ / 2.0f - 20.0f, 50.0f, 40.0f};

  DrawButton(minus_button, "-");
  DrawButton(plus_button, "+");

  char runs_text[32];
  std::snprintf(runs_text, sizeof(runs_text), "%d", selected_runs_);

  int runs_width = MeasureText(runs_text, 32);
  DrawText(runs_text, (screen_width_ - runs_width) / 2, screen_height_ / 2 - 17,
           32, kText);

  Rectangle start_button = {screen_width_ / 2.0f - 100.0f,
                            screen_height_ / 2.0f + 55.0f, 200.0f, 42.0f};

  Rectangle cancel_button = {screen_width_ / 2.0f - 100.0f,
                             screen_height_ / 2.0f + 110.0f, 200.0f, 38.0f};

  DrawButton(start_button, "RUN EXPERIMENT");
  DrawButton(cancel_button, "CANCEL");
  DrawButton({20.0f, static_cast<float>(screen_height_ - kBottomBarHeight + 11),
              150.0f, 36.0f},
             "MAIN MENU");

  const char* hint = "Use +/- or Left/Right arrows, then Enter";
  int hint_width = MeasureText(hint, 16);
  DrawText(hint, (screen_width_ - hint_width) / 2, screen_height_ - 45, 16,
           kMutedText);
}

void Renderer::DrawMultiRunResults() const {
  DrawRectangle(0, 0, screen_width_, screen_height_, kBackground);

  DrawRectangle(0, 0, screen_width_, 70, kPanelDark);
  DrawText("MULTI-RUN RESULTS", 25, 15, 28, kText);

  char run_summary[64];
  std::snprintf(run_summary, sizeof(run_summary), "%d RUNS",
                static_cast<int>(multi_run_costs_.size()));
  DrawText(run_summary, screen_width_ - 125, 21, 16, kMutedText);

  const int footer_height = kBottomBarHeight;
  const int content_top = 70;
  const int content_bottom = screen_height_ - footer_height;
  const int map_width = static_cast<int>(screen_width_ * 0.67f);
  const int panel_x = map_width;
  const int panel_width = screen_width_ - panel_x;

  DrawRectangle(0, content_top, map_width, content_bottom - content_top,
                kWhite);
  DrawRectangle(panel_x, content_top, panel_width, content_bottom - content_top,
                kPanel);

  DrawText("BEST CLUSTERING", 20, content_top + 15, 20, kText);

  if (multi_run_best_run_ >= 0 &&
      multi_run_best_run_ < static_cast<int>(multi_run_costs_.size())) {
    char best_run_text[64];
    std::snprintf(best_run_text, sizeof(best_run_text), "Solution from run %d",
                  multi_run_best_run_ + 1);
    DrawText(best_run_text, 20, content_top + 42, 15, kMutedText);
  }

  DrawLine(20, content_top + 68, map_width - 20, content_top + 68, kBorder);

  Rectangle map_clip = {0.0f, static_cast<float>(content_top + 70),
                        static_cast<float>(map_width),
                        static_cast<float>(content_bottom - content_top - 70)};
  DrawPoints(multi_run_points_, map_clip);
  DrawCentroids(multi_run_centroids_, map_clip);

  DrawText("RESULT SUMMARY", panel_x + 20, content_top + 15, 20, kText);

  double best_cost = 0.0;
  if (!multi_run_costs_.empty() && multi_run_best_run_ >= 0 &&
      multi_run_best_run_ < static_cast<int>(multi_run_costs_.size())) {
    best_cost = multi_run_costs_[multi_run_best_run_];
  }

  Rectangle best_box = {static_cast<float>(panel_x + 15),
                        static_cast<float>(content_top + 52),
                        static_cast<float>(panel_width - 30), 90.0f};

  DrawRectangleRec(best_box, kWhite);
  DrawRectangleLinesEx(best_box, 1.0f, kBorder);

  DrawText("BEST RUN", static_cast<int>(best_box.x) + 15,
           static_cast<int>(best_box.y) + 12, 14, kMutedText);

  char best_run_number[32];
  std::snprintf(best_run_number, sizeof(best_run_number), "Run %d",
                multi_run_best_run_ + 1);
  DrawText(best_run_number, static_cast<int>(best_box.x) + 15,
           static_cast<int>(best_box.y) + 34, 24, kText);

  char best_cost_text[64];
  std::snprintf(best_cost_text, sizeof(best_cost_text), "Cost: %.6f",
                best_cost);
  DrawText(best_cost_text, static_cast<int>(best_box.x) + 105,
           static_cast<int>(best_box.y) + 38, 15, kMutedText);

  const int costs_title_y = content_top + 160;
  DrawText("ALL RUN COSTS", panel_x + 20, costs_title_y, 18, kText);

  const int table_x = panel_x + 20;
  const int table_y = costs_title_y + 35;
  DrawText("RUN", table_x, table_y, 14, kMutedText);
  DrawText("COST", table_x + 70, table_y, 14, kMutedText);

  DrawLine(table_x, table_y + 22, screen_width_ - 20, table_y + 22, kBorder);

  if (multi_run_costs_.empty()) {
    DrawText("No results available.", table_x, table_y + 40, 16, kMutedText);
  } else {
    const int row_height = 25;
    const int list_top = table_y + 32;
    const int list_bottom = content_bottom - 15;
    const int max_rows = std::max(1, (list_bottom - list_top) / row_height);
    const int total_runs = static_cast<int>(multi_run_costs_.size());
    const int max_scroll = std::max(0, total_runs - max_rows);
    const int first_run = std::min(multi_run_cost_scroll_, max_scroll);
    const int last_run = std::min(total_runs, first_run + max_rows);

    for (int i = first_run; i < last_run; ++i) {
      const bool is_best = (i == multi_run_best_run_);
      int row_y = list_top + (i - first_run) * row_height;

      if (is_best) {
        Rectangle highlight = {static_cast<float>(table_x - 8),
                               static_cast<float>(row_y - 3),
                               static_cast<float>(panel_width - 25),
                               static_cast<float>(row_height)};
        DrawRectangleRec(highlight, kWhite);
        DrawRectangleLinesEx(highlight, 1.0f, kBorder);
      }

      char run_text[16];
      std::snprintf(run_text, sizeof(run_text), "%d", i + 1);
      DrawText(run_text, table_x, row_y, 15, kText);

      char cost_text[64];
      std::snprintf(cost_text, sizeof(cost_text), "%.6f", multi_run_costs_[i]);
      DrawText(cost_text, table_x + 70, row_y, 15, kText);

      if (is_best) {
        DrawText("* BEST", screen_width_ - 82, row_y, 13, kText);
      }
    }

    if (max_scroll > 0) {
      char scroll_text[64];
      std::snprintf(scroll_text, sizeof(scroll_text), "%d-%d of %d",
                    first_run + 1, last_run, total_runs);
      int scroll_width = MeasureText(scroll_text, 13);
      DrawText(scroll_text, screen_width_ - scroll_width - 20,
               content_bottom - 18, 13, kMutedText);
    }
  }

  DrawRectangle(0, content_bottom, screen_width_, footer_height, kPanelDark);

  Rectangle menu_button = {20.0f, static_cast<float>(content_bottom + 11),
                           150.0f, 36.0f};
  DrawButton(menu_button, "MAIN MENU");

  DrawText("M / ESC", 190, content_bottom + 13, 14, kText);
  DrawText("Main menu", 190, content_bottom + 32, 13, kMutedText);
  DrawText("Q: Quit", screen_width_ - 80, content_bottom + 21, 14, kText);
}

void Renderer::DrawConvergedScreen(const std::vector<Point>& points,
                                   const std::vector<Centroid>& centroids,
                                   double total_cost, int iteration) const {
  if (show_converged_map_) {
    DrawConvergedMap(points, centroids, total_cost, iteration);
    return;
  }

  DrawRectangle(0, 0, screen_width_, screen_height_, kBackground);

  // ------------------------------------------------------------
  // TITLE
  // ------------------------------------------------------------

  const char* title = "K-MEANS CONVERGED";

  int title_size = 40;

  int title_width = MeasureText(title, title_size);

  DrawText(title, (screen_width_ - title_width) / 2, 70, title_size, kText);

  const char* subtitle = "The cluster assignments are no longer changing.";

  int subtitle_size = 18;

  int subtitle_width = MeasureText(subtitle, subtitle_size);

  DrawText(subtitle, (screen_width_ - subtitle_width) / 2, 125, subtitle_size,
           kMutedText);

  // ------------------------------------------------------------
  // SUMMARY PANEL
  // ------------------------------------------------------------

  float panel_width = 500.0f;
  float panel_height = 250.0f;

  float panel_x = (screen_width_ - panel_width) / 2.0f;

  float panel_y = 175.0f;

  Rectangle panel = {panel_x, panel_y, panel_width, panel_height};

  DrawRectangleRec(panel, kWhite);

  DrawRectangleLinesEx(panel, 2.0f, kBorder);

  DrawText("SUMMARY", static_cast<int>(panel_x + 25),
           static_cast<int>(panel_y + 20), 22, kText);

  char buffer[128];

  // Points
  std::snprintf(buffer, sizeof(buffer), "Points: %d",
                static_cast<int>(points.size()));

  DrawText(buffer, static_cast<int>(panel_x + 25),
           static_cast<int>(panel_y + 65), 18, kMutedText);

  // Clusters
  std::snprintf(buffer, sizeof(buffer), "Clusters: %d",
                static_cast<int>(centroids.size()));

  DrawText(buffer, static_cast<int>(panel_x + 25),
           static_cast<int>(panel_y + 100), 18, kMutedText);

  // Iterations
  std::snprintf(buffer, sizeof(buffer), "Iterations: %d", iteration);

  DrawText(buffer, static_cast<int>(panel_x + 25),
           static_cast<int>(panel_y + 135), 18, kMutedText);

  // Final cost
  std::snprintf(buffer, sizeof(buffer), "Final cost: %.4f", total_cost);

  DrawText(buffer, static_cast<int>(panel_x + 25),
           static_cast<int>(panel_y + 170), 18, kMutedText);

  // ------------------------------------------------------------
  // BUTTONS
  // ------------------------------------------------------------

  const float button_y = screen_height_ / 2.0f + 90.0f;
  Rectangle map_button = {screen_width_ / 2.0f - 270.0f, button_y, 160.0f,
                          45.0f};
  Rectangle reset_button = {screen_width_ / 2.0f - 80.0f, button_y, 160.0f,
                            45.0f};
  Rectangle menu_button = {screen_width_ / 2.0f + 110.0f, button_y, 160.0f,
                           45.0f};
  Rectangle hard_reset_button = {screen_width_ / 2.0f + 300.0f, button_y,
                                 160.0f, 45.0f};

  DrawButton(map_button, "VIEW MAP");
  DrawButton(reset_button, "RESET");
  DrawButton(menu_button, "MAIN MENU");
  DrawButton(hard_reset_button, "HARD RESET");

  // ------------------------------------------------------------
  // KEYBOARD HINTS
  // ------------------------------------------------------------

  const char* hints = "V: View map     R / Enter: Reset     M / Esc: Main Menu";

  int hints_width = MeasureText(hints, 16);

  DrawText(hints, (screen_width_ - hints_width) / 2, screen_height_ - 55, 16,
           kMutedText);
}

void Renderer::DrawConvergedMap(const std::vector<Point>& points,
                                const std::vector<Centroid>& centroids,
                                double total_cost, int iteration) const {
  DrawRectangle(0, 0, screen_width_, screen_height_, kBackground);
  DrawRectangle(0, 0, screen_width_, kTopBarHeight, kPanelDark);
  DrawText("FINAL CLUSTERING MAP", 22, 17, 26, kText);

  DrawRectangle(0, kTopBarHeight, kLeftPanelWidth,
                screen_height_ - kTopBarHeight - kBottomBarHeight, kPanel);
  DrawRectangle(kLeftPanelWidth, kTopBarHeight, screen_width_ - kLeftPanelWidth,
                screen_height_ - kTopBarHeight - kBottomBarHeight, kWhite);

  DrawText("SUMMARY", 18, kTopBarHeight + 18, 18, kText);
  char buffer[128];
  std::snprintf(buffer, sizeof(buffer), "Points: %d",
                static_cast<int>(points.size()));
  DrawText(buffer, 18, kTopBarHeight + 55, 14, kMutedText);
  std::snprintf(buffer, sizeof(buffer), "Clusters: %d",
                static_cast<int>(centroids.size()));
  DrawText(buffer, 18, kTopBarHeight + 80, 14, kMutedText);
  std::snprintf(buffer, sizeof(buffer), "Iterations: %d", iteration);
  DrawText(buffer, 18, kTopBarHeight + 105, 14, kMutedText);
  std::snprintf(buffer, sizeof(buffer), "Cost: %.4f", total_cost);
  DrawText(buffer, 18, kTopBarHeight + 130, 14, kMutedText);
  DrawText("Middle mouse: pan", 18, kTopBarHeight + 175, 13, kMutedText);
  DrawText("Wheel: zoom", 18, kTopBarHeight + 195, 13, kMutedText);

  Rectangle plot_clip = {
      static_cast<float>(kLeftPanelWidth), static_cast<float>(kTopBarHeight),
      static_cast<float>(screen_width_ - kLeftPanelWidth),
      static_cast<float>(screen_height_ - kTopBarHeight - kBottomBarHeight)};
  DrawPoints(points, plot_clip);
  DrawCentroids(centroids, plot_clip);

  const int footer_y = screen_height_ - kBottomBarHeight;
  DrawRectangle(0, footer_y, screen_width_, kBottomBarHeight, kPanelDark);
  const float button_y = static_cast<float>(footer_y + 11);
  DrawButton({20.0f, button_y, 140.0f, 36.0f}, "SUMMARY");
  DrawButton({170.0f, button_y, 100.0f, 36.0f}, "RESET");
  DrawButton({280.0f, button_y, 150.0f, 36.0f}, "MAIN MENU");
  DrawButton({440.0f, button_y, 150.0f, 36.0f}, "HARD RESET");
  DrawText("V: Summary", 455, footer_y + 13, 14, kText);
  DrawText("R / Enter: Reset  |  M / Esc: Main Menu", 455, footer_y + 32, 13,
           kMutedText);
}

void Renderer::DrawHelp() const {
  DrawRectangle(0, 0, screen_width_, screen_height_, Fade(BLACK, 0.55f));

  const float panel_width =
      std::min(680.0f, static_cast<float>(screen_width_) - 40.0f);
  const float panel_height =
      std::min(590.0f, static_cast<float>(screen_height_) - 30.0f);
  const float panel_x = (screen_width_ - panel_width) / 2.0f;
  const float panel_y = (screen_height_ - panel_height) / 2.0f;
  Rectangle panel = {panel_x, panel_y, panel_width, panel_height};
  DrawRectangleRec(panel, kWhite);
  DrawRectangleLinesEx(panel, 2.0f, kBorder);
  DrawText("HELP & CONTROLS", static_cast<int>(panel_x + 22),
           static_cast<int>(panel_y + 16), 26, kText);

  int y = static_cast<int>(panel_y + 58);
  const int left = static_cast<int>(panel_x + 25);
  const int right = static_cast<int>(panel_x + 190);
  auto row = [&](const char* key, const char* description) {
    DrawText(key, left, y, 15, kText);
    DrawText(description, right, y, 15, kMutedText);
    y += 23;
  };
  row("SPACE", "Advance one K-means iteration");
  row("R", "Start or stop automatic running");
  row("ESC", "Soft reset using the same seed");
  row("F2", "Reset map zoom and pan");
  row("Middle mouse", "Pan the map");
  row("Mouse wheel", "Zoom around the cursor");
  row("[ / ]", "Decrease / increase run speed");
  row("M", "Open multi-run experiment setup");
  row("Q", "Quit the application");
  row("Seed field", "Edit the integer seed in setup; press Enter to apply");
  row("Randomize seed", "Choose a new seed for the next initialization");
  row("Empty centroid",
      "A notice lists centroids reinitialized after an iteration");

  y += 7;
  DrawLine(left, y, static_cast<int>(panel_x + panel_width - 25), y, kBorder);
  y += 13;
  DrawText("LOAD POINTS FROM FILE", left, y, 16, kText);
  y += 25;
  Rectangle filename_box = {panel_x + 22, static_cast<float>(y),
                            panel_width - 44, 36.0f};
  DrawRectangleRec(filename_box, kBackground);
  DrawRectangleLinesEx(filename_box, 1.0f, filename_active_ ? kText : kBorder);
  const char* filename = filename_input_.empty()
                             ? "Click here, type filename, then press Enter"
                             : filename_input_.c_str();
  DrawText(filename, static_cast<int>(filename_box.x + 10),
           static_cast<int>(filename_box.y + 9), 15,
           filename_input_.empty() ? kMutedText : kText);
  DrawText("F1 / H / Esc: close help", left,
           static_cast<int>(panel_y + panel_height - 27), 14, kMutedText);
}

void Renderer::FitView(const std::vector<Point>& points,
                       const std::vector<Centroid>& centroids) {
  if (points.empty() && centroids.empty()) {
    zoom_ = 1.0f;
    pan_ = {0.0f, 0.0f};
    return;
  }

  double minimum_x = std::numeric_limits<double>::max();

  double maximum_x = std::numeric_limits<double>::lowest();

  double minimum_y = std::numeric_limits<double>::max();

  double maximum_y = std::numeric_limits<double>::lowest();

  for (const Point& point : points) {
    minimum_x = std::min(minimum_x, point.GetX());

    maximum_x = std::max(maximum_x, point.GetX());

    minimum_y = std::min(minimum_y, point.GetY());

    maximum_y = std::max(maximum_y, point.GetY());
  }

  for (const Centroid& centroid : centroids) {
    minimum_x = std::min(minimum_x, centroid.GetX());

    maximum_x = std::max(maximum_x, centroid.GetX());

    minimum_y = std::min(minimum_y, centroid.GetY());

    maximum_y = std::max(maximum_y, centroid.GetY());
  }

  if (maximum_x <= minimum_x || maximum_y <= minimum_y) {
    zoom_ = 1.0f;
    pan_ = {0.0f, 0.0f};
    return;
  }

  double data_width = maximum_x - minimum_x;

  double data_height = maximum_y - minimum_y;

  double world_width = world_max_ - world_min_;

  double world_height = world_max_ - world_min_;

  if (world_width <= 0.0 || world_height <= 0.0) {
    return;
  }

  float plot_width = static_cast<float>(screen_width_ - kLeftPanelWidth);

  float plot_height =
      static_cast<float>(screen_height_ - kTopBarHeight - kBottomBarHeight);

  float zoom_x =
      plot_width / static_cast<float>(data_width / world_width * plot_width);

  float zoom_y = plot_height /
                 static_cast<float>(data_height / world_height * plot_height);

  zoom_ = std::min(zoom_x, zoom_y) * 0.9f;

  zoom_ = ClampFloat(zoom_, 0.1f, 20.0f);

  Vector2 data_center = {static_cast<float>((minimum_x + maximum_x) / 2.0),
                         static_cast<float>((minimum_y + maximum_y) / 2.0)};

  Vector2 screen_center = {
      kLeftPanelWidth + (screen_width_ - kLeftPanelWidth) / 2.0f,
      kTopBarHeight +
          (screen_height_ - kTopBarHeight - kBottomBarHeight) / 2.0f};

  Vector2 current_center = WorldToScreen(data_center);

  pan_.x += screen_center.x - current_center.x;

  pan_.y += screen_center.y - current_center.y;
}