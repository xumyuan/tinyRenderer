#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char* filename) : verts_(), faces_() {
	std::ifstream in;
	in.open(filename, std::ifstream::in);
	if (in.fail()) return;
	std::string line;
	while (!in.eof()) {
		std::getline(in, line);
		std::istringstream iss(line.c_str());
		char trash;
		if (!line.compare(0, 2, "v ")) {
			iss >> trash;
			Vec3f v;
			for (int i = 0; i < 3; i++) iss >> v.raw[i];
			verts_.push_back(v);
		}
		else if (!line.compare(0, 3, "vt ")) {
			iss >> trash >> trash;
			Vec2f t;
			iss >> t.raw[0];
			iss >> t.raw[1];
			uvs_.push_back(t);
		}
		else if (!line.compare(0, 3, "vn ")) {
			iss >> trash >> trash;
			Vec3f n;
			for (int i = 0; i < 3; i++) iss >> n.raw[i];
			norms_.push_back(n);
		}
		else if (!line.compare(0, 2, "f ")) {
			std::vector<int> f(9);
			int itrash;
			int vdx;	//顶点坐标索引
			int tdx;	//纹理坐标索引
			int ndx;	//法线坐标索引
			iss >> trash;
			int i = 0;
			while (iss >> vdx >> trash >> tdx >> trash >> ndx) {
				vdx--; // obj的索引是从1开始的，而不是0
				tdx--;
				ndx--;
				f[i] = vdx;
				f[i + 3] = tdx;
				f[i + 6] = ndx;
				i++;
				//f.push_back(vdx);
			}
			faces_.push_back(f);
		}
	}
	std::cerr << "# v# " << verts_.size() << " f# " << faces_.size() << std::endl;
	load_texture(filename, "_diffuse.tga", 0);
	load_texture(filename, "_nm.tga", 1);
	load_texture(filename, "_spec.tga", 2);
}

Model::~Model() {
}
// 顶点个数
int Model::nverts() {
	return (int)verts_.size();
}
// 三角形个数
int Model::nfaces() {
	return (int)faces_.size();
}

std::vector<int> Model::face(int idx) {
	return faces_[idx];
}

Vec3f Model::vert(int i) {
	return verts_[i];
}

Vec3f Model::vert(int iface, int nthvert) {
	return verts_[faces_[iface][nthvert]];
}

Vec2f Model::uv(int i) {
	return uvs_[i];
}

Vec2f Model::uv(int iface, int nthvert) {
	return uvs_[faces_[iface][nthvert + 3]];
}

Vec3f Model::norm(int i) {
	return norms_[i];
}

Vec3f Model::norm(int iface, int nthvert) {
	return norms_[faces_[iface][nthvert + 6]];
}

void Model::load_texture(std::string filename, const char* suffix, int tex) {
	std::string texfile(filename);
	size_t dot = texfile.find_last_of(".");
	TGAImage* img = new TGAImage();
	if (dot != std::string::npos) {
		texfile = texfile.substr(0, dot) + std::string(suffix);
		std::cerr << "texture file " << texfile << " loading " << (img->read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
		img->flip_vertically();
		switch (tex)
		{
		case 0:
			diffusemap_ = new Texture(img);
			break;
		case 1:
			normalmap_ = new Texture(img);
			break;
		case 2:
			specularmap_ = new Texture(img);
			break;
		default:
			break;
		}
	}
}

TGAColor Model::diffuse(Vec2f uv) { return diffusemap_->sampleTex(uv); }
TGAColor Model::specular(Vec2f uv) { return specularmap_->sampleTex(uv); }
TGAColor Model::normal(Vec2f uv) { return normalmap_->sampleTex(uv); }

TGAColor Model::diffuse(float u, float v) { return diffusemap_->sampleTex(u, v); }
TGAColor Model::specular(float u, float v) { return specularmap_->sampleTex(u, v); }
TGAColor Model::normal(float u, float v) { return normalmap_->sampleTex(u, v); }
