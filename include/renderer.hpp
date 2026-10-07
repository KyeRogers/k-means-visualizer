#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "raylib.h"

class Renderer {
  public:
    void Initialize(const int window_size);
    void Close() const;
  private:
    int window_size_ = 0;
};

#endif