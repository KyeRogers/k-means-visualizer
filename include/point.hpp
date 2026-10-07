#ifndef POINT_HPP
#define POINT_HPP

class Point {
    public:
      Point();
      Point(const double x, const double y, const int centroid);

      // getters
      double GetX() const;
      double GetY() const;
      int GetCentroid() const;

      // setters 
      void SetX(const double x);
      void SetY(const double y);
      void SetCentroid(const int centroid);
      
    private:
      double x_coordenate_;
      double y_coordenate_;
      int centroid_; // index of which centroid it belongs to
};

#endif
