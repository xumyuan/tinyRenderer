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
			texts_.push_back(t);
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
			int vdx;	//������������
			int tdx;	//������������
			int ndx;	//������������
			iss >> trash;
			int i = 0;
			while (iss >> vdx >> trash >> tdx >> trash >> ndx) {
				vdx--; // obj�������Ǵ�1��ʼ�ģ�������0
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
}

Model::~Model() {
}

// �������
int Model::nverts() {
	return (int)verts_.size();
}
// �����θ���
int Model::nfaces() {
	return (int)faces_.size();
}

std::vector<int> Model::face(int idx) {
	return faces_[idx];
}

Vec3f Model::vert(int i) {
	return verts_[i];
}

Vec2f Model::text(int i) {
	return texts_[i];
}

Vec3f Model::norm(int i) {
	return norms_[i];
}

