#pragma once
#include "tgaimage.h"
#include "geometry.h"
#include "Texture.h"

extern Matrix ModelView;
extern Matrix Viewport;
extern Matrix Projection;

const int depth = 255;
Vec3f m2v(Matrix m);
Matrix v2m(Vec3f v);



struct IShader {
	virtual ~IShader();
	virtual Vec3f vertex(int iface, int nthvert) = 0;
	virtual TGAColor fragment(Vec3f bar) = 0;
};

void triangle(Vec3f* pts, IShader& shader, TGAImage& image, float* zbuffer);
void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color);
void viewport(int x, int y, int w, int h);
void projection(float coeff = 0.f); // coeff = -1/c
void lookat(Vec3f eye, Vec3f center, Vec3f up);
inline void line(Vec2i v0, Vec2i v1, TGAImage& image, TGAColor color) {
	line(v0.x, v0.y, v1.x, v1.y, image, color);
}


