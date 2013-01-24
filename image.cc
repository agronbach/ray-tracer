/**
 * Bob Somers
 * rsomers@calpoly.edu
 * CPE 473, Winter 2010
 * Cal Poly, San Luis Obispo
 */

#include <algorithm>
#include "image.h"
#include "math.h"

Image::Image(int width, int height)
{
    _width = width;
    _height = height;
    _max = 1.0;

    // allocate the first dimension, "width" number of color_t pointers...
    _pixmap = (color_t **)malloc(sizeof(color_t *) * _width);
    // allocate the second dimension, "height" number of color_t structs...
    for (int i = 0; i < _width; i++)
        _pixmap[i] = (color_t *)malloc(sizeof(color_t) * _height);
}

Image::~Image()
{
    // free each column of pixels first...
    for (int i = 0; i < _width; i++)
        free(_pixmap[i]);
    // free the rows of pixels second...
    free(_pixmap);
}

void Image::FillScreen(const color_t& color) {
   for (int i = 0; i < _width; ++i) {
      for (int j = 0; j < _height; ++j)
         _pixmap[i][j] = color;
   }
}

void Image::WriteTga(const char *outfile, bool scale_color) {
    FILE *fp = fopen(outfile, "w");
    if (fp == NULL)
    {
        perror("ERROR: Image::WriteTga() failed to open file for writing!\n");
        exit(EXIT_FAILURE);
    }
    WriteTgaHeader(fp);

    for (int y = 0; y < _height; y++) {
       for (int x = 0; x < _width; x++) {
          WriteColor(fp, _pixmap[x][y], scale_color);
       }
    }

    fclose(fp);
}

void Image::WriteColor(FILE* fp, const color_t& color, bool scale) {
   // write the raw pixel data in groups of 3 bytes (BGR order)
   putc(GetColorByte(color.b, scale), fp);
   putc(GetColorByte(color.g, scale), fp);
   putc(GetColorByte(color.r, scale), fp);
}

unsigned char Image::GetColorByte(double color, bool scale_color) {
   // if color scaling is on, scale 0.0 -> _max as a 0 -> 255 unsigned byte
   if (scale_color)
      return (unsigned char)(color/_max * 255);

   color = std::min(color, 1.0);
   return (unsigned char)(color * 255);
}

void Image::WriteTgaHeader(FILE* fp) {
    // write 24-bit uncompressed targa header
    // thanks to Paul Bourke (http://local.wasp.uwa.edu.au/~pbourke/dataformats/tga/)
    putc(0, fp);
    putc(0, fp);

    putc(2, fp); // type is uncompressed RGB

    putc(0, fp);
    putc(0, fp);
    putc(0, fp);
    putc(0, fp);
    putc(0, fp);

    putc(0, fp);
    putc(0, fp);

    putc(0, fp);
    putc(0, fp);

    putc(_width & 0xff, fp); // width, low byte
    putc((_width & 0xff00) >> 8, fp); // width, high byte

    putc(_height & 0xff, fp); // height, low byte
    putc((_height & 0xff00) >> 8, fp); // height, high byte

    putc(24, fp); // 24-bit color depth

    putc(0, fp);
}

color_t Image::pixel(int x, int y) const
{
    if (out_of_bounds(x, y)) {
        // catostrophically fail
        fprintf(stderr, "ERROR: Image::pixel(%d, %d) outside range of the image!\n", x, y);
        exit(EXIT_FAILURE);
    }

    return _pixmap[x][y];
}

void Image::pixel(int x, int y, color_t pxl)
{
    if (out_of_bounds(x, y)) {
        // catostrophically fail
        fprintf(stderr, "ERROR: Image::pixel(%d, %d, pixel) outside range of the image!\n", x, y);
        exit(EXIT_FAILURE);
    }

    _pixmap[x][y] = pxl;
    UpdateMaxColor(pxl);
}

void Image::UpdateMaxColor(const color_t& color) {
    _max = std::max(std::max(color.r, color.g), std::max(color.b, _max));
}
