#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include <iostream>

#include "Geometry.h"
#include "ShaderProgram.h"

class Line
{
public:
	std::vector<Vertex> verts;
	GPU_Geometry geometry;

	void draw(ShaderProgram& shader) {
		shader.use();
		geometry.bind();
		glDrawArrays(GL_LINE_STRIP, 0, GLsizei(verts.size()));
		glBindVertexArray(0);
	}

	void drawPoints(float pointSize, ShaderProgram& shader) {
		shader.use();
		geometry.bind();
		glPointSize(pointSize);
		glDrawArrays(GL_POINTS, 0, GLsizei(verts.size()));
		glBindVertexArray(0);
	}

	void updateGPU() {
		geometry.bind();
		geometry.setVerts(verts);
	}

	Line(std::vector<Vertex>& v)
		: verts(v)
	{}

	Line()
		: verts()
	{}
};