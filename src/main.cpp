#include <vector> 
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
float* shadowbuffer = nullptr;
const int width = 800;
const int height = 800;
Matrix M;

Texture* depthTexture = nullptr;

Vec3f light_dir = Vec3f(-1, -1, -3).normalize();
Vec3f light_pos = Vec3f(1, 1, 1);
Vec3f eye(1, 1, 3);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

struct Shader :public IShader {
	Vec2f TexCoord[3];
	Vec3f normal[3];
	Vec3f FragPoses[3];
	Vec3f lightSpacePos[3];

	Matrix normalMatrix;
	Matrix lightSpaceMatrix;
	Texture* diffuse;
	Texture* normalMap;
	Texture* specular;
	Texture* shadowMap;

	~Shader() {
		delete diffuse;
		delete normalMap;
		delete specular;
		delete shadowMap;
	}
	Vec3f vertex(int iface, int nthvert) override {
		auto gl_vertex = v2m(model->vert(iface, nthvert));
		FragPoses[nthvert] = m2v(gl_vertex);
		lightSpacePos[nthvert] = m2v(lightSpaceMatrix * v2m(FragPoses[nthvert]));
		gl_vertex = Viewport * Projection * ModelView * gl_vertex;
		TexCoord[nthvert] = model->uv(iface, nthvert);

		return m2v(gl_vertex);
	}

	float calculateShadow(Vec3f lightSpacePos, float angle) {
		auto shadowCoord = lightSpacePos;

		float shadow = 0.0;
		float bias = 28.0f;
		if (shadowCoord.x >= 0 && shadowCoord.x < width && shadowCoord.y >= 0 && shadowCoord.y < height) {
			float closestDepth = shadowbuffer[int(shadowCoord.x + 0.5f) + int(shadowCoord.y + 0.5f) * width];
			bias = std::max(0.05f * (1.0f - angle), bias);
			shadow = shadowCoord.z < closestDepth - bias ? 1.0 : 0.0;
		}
		return shadow;
	}

	TGAColor fragment(Vec3f bar) override
	{
		TGAColor fragColor;
		auto uvs = TexCoord;
		Vec2f uv(0, 0);
		Vec3f FragPos(0.0f, 0.0f, 0.0f);
		Vec3f lsPos(0.0f, 0.0f, 0.0f);
		for (int i = 0; i < 3; i++) {
			uv.u += bar[i] * uvs[i].u;
			uv.v += bar[i] * uvs[i].v;

			// 片段位置
			FragPos.x += bar[i] * FragPoses[i].x;
			FragPos.y += bar[i] * FragPoses[i].y;
			FragPos.z += bar[i] * FragPoses[i].z;

			// 光照空间位置
			lsPos.x += bar[i] * lightSpacePos[i].x;
			lsPos.y += bar[i] * lightSpacePos[i].y;
			lsPos.z += bar[i] * lightSpacePos[i].z;
		}
		// 光照方向
		//auto lightDir = Vec3f(-light_dir.x, -light_dir.y, -light_dir.z).normalize();
		auto lightDir = (light_pos - FragPos).normalize();
		// 漫反射光强度
		auto diff = diffuse->sampleTex(uv);
		// 镜面光强度
		auto spec = specular->sampleTex(uv).b;
		// 法线贴图
		auto n = normalMap->sampleTex(uv);

		Vec3f normal = Vec3f(2 * n.r - 1, 2 * n.g - 1, 2 * n.b - 1).normalize();



		Vec3f viewDir = (eye - FragPos).normalize();
		auto halfDir = (lightDir + viewDir).normalize();
		normal = m2v(normalMatrix * v2m(normal)).normalize();

		float specStrength = pow(std::max(0.0f, normal * halfDir), spec);
		float diffStrength = std::max(0.0f, normal * lightDir);
		float ambientStrength = 0.3;

		float angle = normal * lightDir;

		float shadow = calculateShadow(lsPos, angle);
		//shadow *= 0.5;
		fragColor = diff * diffStrength * 1.0 * (1.0 - shadow) + TGAColor(spec, spec, spec, 255) * specStrength * 0.6 * (1.0 - shadow) + diff * ambientStrength;

		return fragColor;
	}

};

struct DepthShader :IShader {
	Vec3f vec[3];
	~DepthShader() {}
	Vec3f vertex(int iface, int nthvert) override {
		auto gl_vertex = v2m(model->vert(iface, nthvert));
		gl_vertex = Viewport * Projection * ModelView * gl_vertex;
		vec[nthvert] = m2v(gl_vertex);
		return m2v(gl_vertex);
	}

	TGAColor fragment(Vec3f bar) override {
		// 根据重心坐标计算片段深度
		float z = 0;
		for (int i = 0; i < 3; i++) {
			z += vec[i].z * bar[i];
		}
		return TGAColor(z, z, z, 255);
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

void renderDepth(Model* model, TGAImage& image) {
	shadowbuffer = new float[width * height];
	lookat(light_pos, center, up);
	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	projection(0);

	DepthShader depthShader;
	for (int i = 0; i < model->nfaces(); i++) {
		Vec3f screen_coords[3];
		for (int j = 0; j < 3; j++) {
			screen_coords[j] = depthShader.vertex(i, j);
		}
		triangle(screen_coords, depthShader, image, shadowbuffer);
	}
	// 随机对比100个 image和shadowbuffer的值
	/*for (int i = 0; i < 100; i++) {
		int x = rand() % width;
		int y = rand() % height;
		float z = shadowbuffer[x + y * width];
		std::cout << "depth: " << z << " " << (int)image.get(x, y).r << std::endl;
	}*/
}

void renderTriangleModel(Model* model, TGAImage& image) {
	// ModelView矩阵
	lookat(eye, center, up);
	// 视口变换矩阵
	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	// 投影矩阵
	projection(-1.f / (eye - center).norm());

	//法线矩阵
	Matrix normalMIT = (Projection * ModelView).inverse().transpose();

	// zbuffer 初始化
	float* zbuffer = new float[width * height];
	for (int i = width * height; i--; zbuffer[i] = -std::numeric_limits<float>::max());

	Shader shader;
	shader.diffuse = model->getDiffuse();
	shader.normalMap = model->getNormal();
	shader.specular = model->getSpec();
	shader.normalMatrix = normalMIT;
	shader.shadowMap = depthTexture;
	shader.lightSpaceMatrix = M;

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
	TGAImage depth(width, height, TGAImage::RGB);
	renderDepth(model, depth);
	depth.flip_vertically(); // 将图像原点（0，0）放在左下角
	depth.write_tga_file("resImg/depth.tga");

	depthTexture = new Texture(&depth);

	M = Viewport * Projection * ModelView;

	TGAImage image(width, height, TGAImage::RGB);
	renderTriangleModel(model, image);
	image.flip_vertically(); // 将图像原点（0，0）放在左下角 
	image.write_tga_file("resImg/head_shadow.tga");

	delete model;
	return 0;
}