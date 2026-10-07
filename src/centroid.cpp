#include "centroid.hpp"

Centroid::Centroid(const int x, const int y) : x_coordenate_{x}, y_coordenate_{y} {}

// getters
int Centroid::GetX() const { return x_coordenate_; }
int Centroid::GetY() const { return y_coordenate_; }

// setters
void Centroid::SetX(const int x) { x_coordenate_ = x; }
void Centroid::SetY(const int y) { y_coordenate_ = y; }