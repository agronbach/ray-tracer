/**
 * Bob Somers
 * rsomers@calpoly.edu
 * CPE 473, Winter 2010
 * Cal Poly, San Luis Obispo
 */

#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <stdio.h>
#include <stdlib.h>
#include <glm/glm.hpp>

typedef glm::vec3 color_t;

class Image {
public:
    Image(int width, int height);
    ~Image();

    // if scale_color is true, the output targa will have its color space scaled
    // to the global max, otherwise it will be clamped at 1.0
    void WriteTga(const char *outfile, bool scale_color = false);
    void FillScreen(const color_t& color);

    // property accessors
    color_t pixel(int x, int y) const;
    void pixel(int x, int y, color_t pxl);
    int width() const { return _width; }
    int height() const { return _height; }
    float max() const { return _max; }

private:
    void WriteTgaHeader(FILE* fp);
    void WriteColor(FILE* fp, const color_t& color, bool scale);
    unsigned char GetColorByte(double color, bool scale_color);
    bool out_of_bounds(int x, int y) const {
       return x < 0 || x >= _width || y < 0 || y >= _height;
    }
    void UpdateMaxColor(const color_t& color);

    int _width;
    int _height;
    color_t **_pixmap;
    float _max;
};

#endif
