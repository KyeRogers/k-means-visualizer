#include "centroid.hpp"

Centroid::Centroid() : x_coordenate_{0}, y_coordenate_{0} {}
Centroid::Centroid(const double x, const double y) : x_coordenate_{x}, y_coordenate_{y} {}

// getters
double Centroid::GetX() const { return x_coordenate_; }
double Centroid::GetY() const { return y_coordenate_; }

// setters
void Centroid::SetX(const double x) { x_coordenate_ = x; }
void Centroid::SetY(const double y) { y_coordenate_ = y; }