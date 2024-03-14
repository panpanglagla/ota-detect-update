#ifndef CSS_H
#define CSS_H
#include <Arduino.h>

class Css {
  private:
    String _css;
  public:
    Css();
    String getCSS();
};
#endif