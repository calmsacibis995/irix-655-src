#ifndef VK_EZ_H
#define VK_EZ_H

#include <Xm/Xm.h>


class EZWrapper;

class EZRes {
  public:
    EZRes() { }
    virtual ~EZRes() {}
    inline void init(EZWrapper *ez, char *res);
    
  protected:
    EZWrapper *_ez;
    char      *_resource;
    Widget    _w;
};

class EZDimension : public EZRes {
  public:
    EZDimension() : EZRes() {}
    operator Dimension();
    EZWrapper& operator =(int d);
};

class EZPos : public EZRes {
  public:
    EZPos() :EZRes() {}
    operator Position();
    EZWrapper& operator =(int d);
};

class EZInt : public EZRes {
  public:
    EZInt() : EZRes() {}
    operator int();
    EZWrapper& operator=(int i);
};    

class EZColor : public EZRes {
  public:
    EZColor() : EZRes() { }
    operator Pixel();
    EZWrapper& operator=(char * color);
    EZWrapper& operator=(Pixel color);
};

class EZXmString : public EZRes {
  public:
    EZXmString()  : EZRes() { }
    operator String();
    EZWrapper& operator =(char * label);
};


class EZWrapper {

 private:
    
    Widget _w;

    static EZWrapper* _objList;
    static int _index;
    
   public:

    // to recyle objects after 20, to avoid undue leakage
    
    void *operator new(size_t);
    void operator delete ( void *, size_t );

    
    EZWrapper(Widget w);
    ~EZWrapper();
    EZWrapper& operator=(int);
    EZWrapper& operator=(float);
    EZWrapper& operator=(const char *);
    EZWrapper& operator-=(int);    
    EZWrapper& operator+=(int);
    EZWrapper& operator+=(float);
    EZWrapper& operator+=(const char *);
    EZWrapper& operator<<(int);
    EZWrapper& operator<<(float);
    EZWrapper& operator<<(const char *);
    operator int() { return getint();}
    operator String() { return getstring();}
    int getint();
    char *getstring();
    Widget w() { return _w;}

    EZDimension border;
    EZDimension width;    
    EZDimension height;
    EZPos       x;
    EZPos       y;
    EZColor     foreground;
    EZColor     background;
    EZXmString  label;
    EZInt       value;
    EZInt       minimum;
    EZInt       maximum;
};


// provide a convenience function because C++ doesn't like to instantiate
// unamed objects.

extern EZWrapper &EZ(Widget wrappee);
    
void InitEZ(); // Just to force a link to happen

#endif

