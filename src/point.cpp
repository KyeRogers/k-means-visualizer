#include "point.hpp"

Point::Point(const int x, const int y, const int centroid)
    : x_coordenate_{x}, y_coordenate_{y}, centroid_{centroid} {}

// getters
int Point::GetX() const { return x_coordenate_; }
int Point::GetY() const { return y_coordenate_; }
int Point::GetCentroid() const { return centroid_; }

// setters
void Point::SetX(const int x) { x_coordenate_ = x; }
void Point::SetY(const int y) { y_coordenate_ = y; }
void Point::SetCentroid(const int centroid) { centroid_ = centroid; }