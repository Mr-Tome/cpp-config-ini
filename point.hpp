#include<stdio.h>


class point{
    double x;
    double y; // an object-oriented implementation
public:
    //initializes x and y with xx and yy.
    //if no arguments are provded, xx=0 and yy=0
    point(double xx=0., double yy=0.):x(xx),y(yy){};
    ~point(){};
    double X() const;
    double Y() const;
    const point* zero();

public:
    //provides the negative of the in put argument.
    const point negative(const point& p);
};

double point::X() const{
    return x;
} // read x

double point::Y() const{
    return y;
} // read y

const point* point::zero(){
    x=y=0.;
    return this;
} // set to zero

const point point::negative(const point& p)
{
    return point(-p.X(), -p.Y());
} 