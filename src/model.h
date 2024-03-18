#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include <string>

#include "geometry.h"
#include "tgaimage.h"
#include "Texture.h"

class Model {
private:
	std::vector<std::vector<int> > faces_; //ÿ��face�Ÿ�Ԫ��  ����һ�� �ֱ�Ϊ vert/uv/norm
	std::vector<Vec3f> verts_;	// ������Ϣ
	std::vector<Vec2f> uvs_;	// ����������Ϣ
	std::vector<Vec3f> norms_;	// ������Ϣ
	Texture* diffusemap_;
	Texture* normalmap_;
	Texture* specularmap_;
	void load_texture(std::string filename, const char* suffix, int tex);
public:
	Model(const char* filename);
	~Model();
	int nverts();
	int nfaces();
	Vec3f vert(int i);
	Vec3f vert(int iface, int nthvert);

	Vec2f uv(int i);
	Vec2f uv(int iface, int nthvert);

	Vec3f norm(int i);
	Vec3f norm(int iface, int nthvert);
	std::vector<int> face(int idx);

	TGAColor diffuse(Vec2f uv);
	TGAColor specular(Vec2f uv);
	TGAColor normal(Vec2f uv);

	TGAColor diffuse(float u, float v);
	TGAColor specular(float u, float v);
	TGAColor normal(float u, float v);

	inline Texture* getDiffuse() { return diffusemap_; }

};

#endif //__MODEL_H__
