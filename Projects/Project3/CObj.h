#pragma once

#ifndef __COBJ__
#define __COBJ__

#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include "CTexture.h"
#include "CShader.h"

class CObj {
public:
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;

	std::vector<unsigned short> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;

	bool bufferExisted;
	GLuint vertexbuffer;
	GLuint uvbuffer;
	GLuint normalbuffer;
	GLuint elementbuffer;

	CObj();
	~CObj();
	void getIndex();
	void getBuffer();
	void CObj::enableAttr(CShader & shader);
	void CObj::disableAttr();
	void CObj::draw(GLenum mode);

	void loadTexture(const char *, CShader &);
	CTexture texture;
};

#endif