#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"

class Model {
private:
	std::vector<Vec3f> verts_;	// ������Ϣ
	std::vector<Vec2f> texts_;	// ����������Ϣ
	std::vector<Vec3f> norms_;	// ������Ϣ
	std::vector<std::vector<int> > faces_;
public:
	Model(const char* filename);
	~Model();
	int nverts();
	int nfaces();
	Vec3f vert(int i);
	Vec2f text(int i);
	Vec3f norm(int i);
	std::vector<int> face(int idx);
};

#endif //__MODEL_H__
