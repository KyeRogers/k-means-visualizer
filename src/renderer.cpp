#include "renderer.hpp"


/** @brief initializes a raylib window */
void Renderer::Initialize(const int window_size) {
  window_size_ = window_size;
  InitWindow(window_size, window_size, "K-MEANS VISUALIZER");
  SetTargetFPS(60);
  SetExitKey(KEY_NULL);
}

/** @brief Closes the raylib window */

void Renderer::Close() const {
  CloseWindow();
}