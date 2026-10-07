#ifndef CENTROID_HPP
#define CENTROID_HPP

class Centroid {
  public:
    Centroid(const int x, const int y);

    // getters 
    int GetX() const;
    int GetY() const;

    // setters 
    void SetX(const int x);
    void SetY(const int y);

  private:
    int x_coordenate_;
    int y_coordenate_;
};

#endif