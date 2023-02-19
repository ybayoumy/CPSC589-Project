#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include <iostream>

#include "Geometry.h"
#include "ShaderProgram.h"

class Mesh
{
public:
	std::vector<Vertex> verts;
	std::vector<unsigned int> indices;

	GPU_Geometry geometry;

	void draw(ShaderProgram& shader) {
		shader.use();
		geometry.bind();
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	void updateGPU() {
		geometry.bind();
		geometry.setVerts(verts);
		geometry.setIndices(indices);
	}

	Mesh(std::vector<Vertex>& v, std::vector<unsigned int>& i)
		: verts(v)
		, indices(i)
	{}

	Mesh()
		: verts()
		, indices() 
	{}
};