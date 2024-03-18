﻿#include <vector> 
#include <iostream>
#include "geometry.h"
#include "tgaimage.h"
#include "model.h"
#include "Texture.h"
#include "our_gl.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);

Model* model = NULL;
const int width = 800;
const int height = 800;

Vec3f light_dir = Vec3f(0, 0, -1).normalize();
Vec3f camera(0, 0, 3);
Vec3f eye(1, 1, 3);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

struct Shader :public IShader {
	Vec2f TexCoord[3];
	Vec3f normal[3];
	Texture* diffuse;

	~Shader() {}
	Vec3f vertex(int iface, int nthvert) override {
		auto gl_vertex = v2m(model->vert(iface, nthvert));
		gl_vertex = Viewport * Projection * ModelView * gl_vertex;
		TexCoord[nthvert] = model->uv(iface, nthvert);
		return m2v(gl_vertex);
	}

	TGAColor fragment(Vec3f bar) override
	{
		TGAColor fragColor;
		auto uvs = TexCoord;
		Vec2f uv(0, 0);
		for (int i = 0; i < 3; i++) {
			uv.u += bar[i] * uvs[i].u;
			uv.v += bar[i] * uvs[i].v;
		}

		auto c = diffuse->sampleTex(uv);
		fragColor = c;
		return fragColor;
	}

};

void renderLineModel(Model* model, TGAImage& image) {
	for (int i = 0; i < model->nfaces(); i++) {
		std::vector<int> face = model->face(i);
		for (int j = 0; j < 3; j++) {
			Vec3f v0 = model->vert(face[j]);
			Vec3f v1 = model->vert(face[(j + 1) % 3]);
			int x0 = (v0.x + 1.) * width / 2.;
			int y0 = (v0.y + 1.) * height / 2.;
			int x1 = (v1.x + 1.) * width / 2.;
			int y1 = (v1.y + 1.) * height / 2.;
			line(x0, y0, x1, y1, image, white);
		}
	}
}

Vec3f world2screen(Vec3f v) {
	return Vec3f(int((v.x + 1.) * width / 2. + .5), int((v.y + 1.) * height / 2. + .5), v.z);
}

void renderTriangleModel(Model* model, TGAImage& image) {
	// ModelView矩阵
	lookat(eye, center, Vec3f(0, 1, 0));
	// 视口变换矩阵
	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	// 投影矩阵
	projection(-1.f / (eye - center).norm());

	std::cerr << ModelView << std::endl;
	std::cerr << Projection << std::endl;
	std::cerr << Viewport << std::endl;
	Matrix z = (Viewport * Projection * ModelView);
	std::cerr << z << std::endl;

	// zbuffer 初始化
	float* zbuffer = new float[width * height];
	for (int i = width * height; i--; zbuffer[i] = -std::numeric_limits<float>::max());

	Shader shader;
	shader.diffuse = model->getDiffuse();
	for (int i = 0; i < model->nfaces(); i++) {
		Vec3f screen_coords[3];
		for (int j = 0; j < 3; j++) {
			screen_coords[j] = shader.vertex(i, j);
		}
		triangle(screen_coords, shader, image, zbuffer);
	}

	delete[]zbuffer;
}

int main(int argc, char** argv) {
	if (2 == argc) {
		model = new Model(argv[1]);
	}
	else {
		model = new Model("model/african_head.obj");
	}

	TGAImage image(width, height, TGAImage::RGB);
	renderTriangleModel(model, image);
	image.flip_vertically(); // 将图像原点（0，0）放在左下角 
	image.write_tga_file("resImg/head_shader.tga");

	delete model;
	return 0;
}