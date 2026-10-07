#ifndef CENTROID_HPP
#define CENTROID_HPP

class Centroid {
  public:
    Centroid();
    Centroid(const double x, const double y);

    // getters 
    double GetX() const;
    double GetY() const;

    // setters 
    void SetX(const double x);
    void SetY(const double y);

  private:
    double x_coordenate_;
    double y_coordenate_;
};

#endif