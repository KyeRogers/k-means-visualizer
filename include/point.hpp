#ifndef POINT_HPP
#define POINT_HPP

class Point {
    public:
      Point(const int x, const int y, const int centroid);

      // getters
      int GetX() const;
      int GetY() const;
      int GetCentroid() const;

      // setters 
      void SetX(const int x);
      void SetY(const int y);
      void SetCentroid(const int centroid);
      
    private:
      int x_coordenate_;
      int y_coordenate_;
      int centroid_; // index of which centroid it belongs to
};

#endif
