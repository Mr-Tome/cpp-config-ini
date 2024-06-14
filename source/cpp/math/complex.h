#include<stdio.h>

class complex
{
    double real; // the real part
    double imaginary; // the imaginary part

public:
    complex(double r=0.,double i=0.)
    :real(r),
    imaginary(i)
    {} // constructor

    complex(const complex& c) 
    :real(c.real),
    imaginary(c.imaginary) {} // copy constructor

    ~complex(){} // destructor

    double re() const
    {
        return this->real;
    } // read real part

    double im() const
    {
        return this->imaginary;
    } // read imaginary part

    // Returns a Complex Number who's private members are the
    // equal to the arguments Complex Number's real and imaginary private members
    const complex& operator=(const complex& c)
    {
        this->real = c.real;
        this->imaginary = c.imaginary;
        return *this;
    } 

    //add complex to the current complex
    const complex& operator+=(const complex& c)
    {
        this->real += c.real;
        this->imaginary += c.imaginary;
        return *this;
    } 

    // subtract complex from the current complex
    const complex& operator-=(const complex& c)
    {
        this->real -= c.real;
        this->imaginary -= c.imaginary;
        return *this;
    } 
    
    // multiply the current complex by a complex
    const complex& operator*=(const complex& c)
    {
        double keepreal = this->real;

        this->real = this->real * c.real - imaginary * c.imaginary;
        this->imaginary = keepreal*c.imaginary + imaginary*c.real;
        
        return *this;
    } 

    // divide the current complex by a real number
    const complex& operator/=(double d)
    {
        this->real /= d;
        this->imaginary /= d;
        return *this;
    } 

    // conjugate of a complex
    friend complex operator!(const complex& c)
    {
        return complex(c.re(),-c.im());
    } 

    // square of the absolute value of a complex
    //The absolute square of a complex number is calculated by multiplying it by its complex conjugate
    friend double abs2(const complex& c)
    {
        return c.re() * c.re() + c.im() * c.im();
    } 

    // divide the current complex by a complex
    //To divide a complex number a+ib by c+id, multiply the numerator and denominator of the fraction a+ib/c+id by c−id and simplify
    const complex& operator/=(const complex& c)
    {
        return *this *= (!c) /= abs2(c);
    } 


    // negative of a complex number
    const complex operator-()
    {
        return complex(-this->re(),this->im());
    } 

    // subtraction of two complex numbers
    friend const complex operator-(const complex& c, const complex& d)
    {
        return complex(c.re() - d.re(),c.im() - d.im());
    } 

    // addition of two complex numbers
    friend const complex operator+(const complex& c,const complex& d)
    {
        return complex(c.re()+d.re(),c.im()+d.im());
    } 

    // multiplication of two complex numbers
    friend const complex operator*(const complex& c,const complex& d)
    {
        return complex(c) *= d;
    } 

    // division of two complex numbers
    friend inline const complex operator/(const complex& c,const complex& d)
    {
        return complex(c) /= d;
    } 

    static void print(std::string s, const complex& c)
    {
        std::cout << s << ": (" << c.re() << ","<<c.im() << ")\n";
        //printf("%f: (%f,%f)\n",s,c.re(),c.im());
    } // printing a complex number

};