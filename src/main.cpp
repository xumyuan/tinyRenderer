#include <vector> 
#include <iostream>
#include "geometry.h"
#include "tgaimage.h"
#include "model.h"
#include "Texture.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);

Model* model = NULL;
TGAImage tex;
const int width = 800;
const int height = 800;

void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color) {
	bool steep = false;
	//对比直线的斜率，如果斜率大于1，就转置
	if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
		std::swap(x0, y0);
		std::swap(x1, y1);
		steep = true;
	}
	//保证起点在终点的左边
	if (x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}
	int dx = x1 - x0;
	int dy = y1 - y0;
	//计算x变化1时，y的变化量
	//如使用dy/dx，累加y的变化量超过0.5，y就加1
	//使用2*dy，累加y的变化量超过dx，y就加1
	//这样做的优势是没有乘除法，计算更快
	int derror2 = std::abs(dy) * 2;
	int error2 = 0;
	int y = y0;
	for (int x = x0; x <= x1; x++) {
		if (steep) {
			image.set(y, x, color);
		}
		else {
			image.set(x, y, color);
		}
		error2 += derror2;
		if (error2 > dx) {
			y += (y1 > y0 ? 1 : -1);
			error2 -= dx * 2;
		}
	}
}

void line(Vec2i v0, Vec2i v1, TGAImage& image, TGAColor color) {
	line(v0.x, v0.y, v1.x, v1.y, image, color);
}

// 求解重心坐标
Vec3f barycentric(Vec3f* pts, Vec3f P) {
	auto A = pts[0];
	auto B = pts[1];
	auto C = pts[2];
	Vec3f s[2];
	for (int i = 2; i--; ) {
		s[i][0] = C[i] - A[i];
		s[i][1] = B[i] - A[i];
		s[i][2] = A[i] - P[i];
	}
	Vec3f u = s[0] ^ s[1];
	if (std::abs(u[2]) > 1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
		return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
	return Vec3f(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
}

void triangle(Vec3f* pts, Vec2f* uvs, TGAImage& image, TGAColor color, float* zbuffer) {
	Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			bboxmin[j] = std::max(0.f, std::min(bboxmin[j], pts[i][j]));
			bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
		}
	}

	Vec3f P;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			Vec3f bc_screen = barycentric(pts, P);
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;
			P.z = 0;
			for (int i = 0; i < 3; i++) P.z += pts[i][2] * bc_screen[i];
			if (zbuffer[int(P.x + P.y * width)] < P.z) {
				zbuffer[int(P.x + P.y * width)] = P.z;
				Vec2f uv(0, 0);
				for (int i = 0; i < 3; i++) {
					uv.u += bc_screen[i] * uvs[i].u;
					uv.v += bc_screen[i] * uvs[i].v;
				}
				auto c = Texture(tex, uv.u, uv.v);

				image.set(P.x, P.y, c);
			}
		}
	}
}

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
	Vec3f light_dir(0, 0, -1);
	// zbuffer 初始化
	float* zbuffer = new float[width * height];
	for (int i = width * height; i--; zbuffer[i] = -std::numeric_limits<float>::max());

	for (int i = 0; i < model->nfaces(); i++) {
		std::vector<int> face = model->face(i);
		Vec3f screen_coords[3];
		Vec3f world_coords[3];
		Vec2f uvs[3];
		for (int j = 0; j < 3; j++) {
			// 顶点坐标
			world_coords[j] = model->vert(face[j]);
			// 纹理坐标
			uvs[j] = model->text(face[j + 3]);
			// 屏幕坐标
			screen_coords[j] = world2screen(world_coords[j]);
		}
		Vec3f n = (world_coords[2] - world_coords[0]) ^
			(world_coords[1] - world_coords[0]);

		n.normalize();

		float intensity = n * light_dir;

		if (intensity > 0)
			triangle(screen_coords, uvs, image,
				TGAColor(intensity * 255, intensity * 255, intensity * 255, 255),
				zbuffer);
	}
}

int main(int argc, char** argv) {
	if (2 == argc) {
		model = new Model(argv[1]);
	}
	else {
		model = new Model("model/african_head.obj");
	}

	tex.read_tga_file("model/african_head_diffuse.tga");
	tex.flip_vertically();

	TGAImage image(width, height, TGAImage::RGB);
	renderTriangleModel(model, image);
	image.flip_vertically(); // 将图像原点（0，0）放在左下角 
	image.write_tga_file("resImg/head_texture.tga");

	return 0;
}