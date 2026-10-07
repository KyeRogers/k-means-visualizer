#include "point.hpp"

Point::Point() : x_coordenate_{0}, y_coordenate_{0}, centroid_{0} {}

Point::Point(const double x, const double y, const int centroid)
    : x_coordenate_{x}, y_coordenate_{y}, centroid_{centroid} {}

// getters
double Point::GetX() const { return x_coordenate_; }
double Point::GetY() const { return y_coordenate_; }
int Point::GetCentroid() const { return centroid_; }

// setters
void Point::SetX(const double x) { x_coordenate_ = x; }
void Point::SetY(const double y) { y_coordenate_ = y; }
void Point::SetCentroid(const int centroid) { centroid_ = centroid; }