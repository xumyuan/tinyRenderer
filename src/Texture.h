#pragma once
#include "tgaimage.h"
#include "geometry.h"

class Texture {
private:
	TGAImage* tex_data;
public:
	Texture(TGAImage* img) {
		tex_data = img;
	}
	~Texture() {}

	TGAColor sampleTex(float u, float v) {
		if (u < 0) u = 0;
		if (u > 1)u = 1;
		if (v < 0)v = 0;
		if (v > 1)v = 1;

		int width = tex_data->get_width();
		int	height = tex_data->get_height();

		float x = width * u;
		float y = height * v;

		// 计算坐标邻近的四个点
		int x0 = static_cast<int>(std::floor(x + 0.5f)) - 1;
		int y0 = static_cast<int>(std::floor(y + 0.5f)) - 1;
		x0 = x0 < 0 ? 0 : x0;
		y0 = y0 < 0 ? 0 : y0;

		int x1 = x0 + 1;
		x1 = x1 >= width ? width - 1 : x1;
		int y1 = y0 + 1;
		y1 = y1 >= height ? height - 1 : y1;

		auto v0 = tex_data->get(x0, y0),
			v1 = tex_data->get(x0, y1),
			v2 = tex_data->get(x1, y0),
			v3 = tex_data->get(x1, y1);
		Vec3f v0_(v0.r, v0.g, v0.b);
		Vec3f v1_(v1.r, v1.g, v1.b);
		Vec3f v2_(v2.r, v2.g, v2.b);
		Vec3f v3_(v3.r, v3.g, v3.b);

		auto temp1 = v0_ + (v2_ - v0_) * (float)(x - (float)x0 - 0.5f);
		auto temp2 = v1_ + (v3_ - v1_) * (float)(x - (float)x0 - 0.5f);

		auto value = temp1 + (temp2 - temp1) * (float)(y - (float)y0 - 0.5f);

		TGAColor res(
			(unsigned char)(value.x + 0.5),
			(unsigned char)(value.y + 0.5),
			(unsigned char)(value.z + 0.5),
			v0.a);
		return res;
	}

	TGAColor sampleTex(Vec2f uv) {
		return sampleTex(uv.u, uv.v);
	}
};