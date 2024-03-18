#include <cmath>
#include <limits>
#include <cstdlib>
#include "our_gl.h"

Matrix ModelView;
Matrix Viewport;
Matrix Projection;

IShader::~IShader() {}

// 将齐次坐标转为三维向量
Vec3f m2v(Matrix m) {
	return Vec3f(
		m[0][0] / m[3][0],
		m[1][0] / m[3][0],
		m[2][0] / m[3][0]);
}

// 将三维向量转为齐次坐标
Matrix v2m(Vec3f v) {
	Matrix m(4, 1);
	m[0][0] = v.x;
	m[1][0] = v.y;
	m[2][0] = v.z;
	m[3][0] = 1.f;
	return m;
}

// 视口变换矩阵
void viewport(int x, int y, int w, int h) {
	Viewport = Matrix::identity(4);
	Viewport[0][3] = x + w / 2.f;
	Viewport[1][3] = y + h / 2.f;
	Viewport[2][3] = 255.f / 2.f;
	Viewport[0][0] = w / 2.f;
	Viewport[1][1] = h / 2.f;
	Viewport[2][2] = 255.f / 2.f;
}

// Modelview矩阵
void lookat(Vec3f eye, Vec3f center, Vec3f up) {
	Vec3f z = (eye - center).normalize();
	Vec3f x = (up ^ z).normalize();
	Vec3f y = (z ^ x).normalize();
	ModelView = Matrix::identity(4);
	for (int i = 0; i < 3; i++) {
		ModelView[0][i] = x[i];
		ModelView[1][i] = y[i];
		ModelView[2][i] = z[i];
		ModelView[i][3] = -center[i];
	}
}

void projection(float coeff) {
	Projection = Matrix::identity(4);
	Projection[3][2] = coeff;
}

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

// 求解重心坐标
Vec3f barycentric(Vec3f* pts, Vec2f P) {
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

// 透视矫正
Vec2f uvCorrecion(Vec3f* worlds, Vec3f bc_screen, Vec2f* uvs) {
	auto alpha = bc_screen[0] / worlds[0][2];
	auto beta = bc_screen[1] / worlds[1][2];
	auto gamma = bc_screen[2] / worlds[2][2];
	auto denominator = alpha + beta + gamma;

	Vec2f uv((alpha * uvs[0].u + beta * uvs[1].u + gamma * uvs[2].u) / denominator,
		(alpha * uvs[0].v + beta * uvs[1].v + gamma * uvs[2].v) / denominator);
	return uv;
}

// 三角形光栅化
void triangle(Vec3f* pts, IShader& shader, TGAImage& image, float* zbuffer) {
	/*for (int i = 0; i < 3; i++) {
		pts[i].x = static_cast<int>(pts[i].x + 0.5f);
		pts[i].y = static_cast<int>(pts[i].y + 0.5f);
	}*/
	auto width = image.get_width();
	auto height = image.get_height();
	Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			bboxmin[j] = std::max(0.f, std::min(bboxmin[j], pts[i][j]));
			bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
		}
	}

	Vec2i P;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			Vec3f bc_screen = barycentric(pts, P);
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;
			float z = 0.0f;
			for (int i = 0; i < 3; i++) z += (float)bc_screen[i] * pts[i][2];
			//P.z = 1.0f / P.z;
			if (zbuffer[int(P.x + P.y * width)] < z) {
				zbuffer[int(P.x + P.y * width)] = z;
				auto c = shader.fragment(bc_screen);
				image.set(P.x, P.y, c);
			}
		}
	}
}