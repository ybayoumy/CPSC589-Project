#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include <iostream>

#include "Geometry.h"
#include "ShaderProgram.h"
#include "Line.h"

bool xdescend(glm::vec3 vec1, glm::vec3 vec2) {
	return (vec1.x < vec2.x);
}

bool ydescend(glm::vec3 vec1, glm::vec3 vec2) {
	return (vec1.y < vec2.y);
}

void orderlines(std::vector<glm::vec3>& Line1, std::vector<glm::vec3>& Line2) {
	float dist1 = glm::distance(Line1[0], Line2[0]);
	float dist2 = glm::distance(Line1[0], Line2[Line2.size() - 1]);
	float dist3 = glm::distance(Line1[Line1.size() - 1], Line2[0]);
	float dist4 = glm::distance(Line1[Line1.size() - 1], Line2[Line2.size() - 1]);

	float distances[4] = { dist1, dist2, dist3, dist4 };

	int min = std::distance(std::begin(distances), std::min_element(std::begin(distances), std::end(distances)));

	if (min == 0) {

	}
	else if (min == 1) {
		std::reverse(Line2.begin(), Line2.end());
	}
	else if (min == 2) {
		std::reverse(Line1.begin(), Line1.end());
	}
	else {
		std::reverse(Line1.begin(), Line1.end());
		std::reverse(Line2.begin(), Line2.end());
	}
}

class Mesh
{
public:
	std::vector<Vertex> verts;
	std::vector<unsigned int> indices;

	GPU_Geometry geometry;

	void create(Line l1, Line l2, int sprecision, glm::vec3 col, std::vector<glm::vec3> sweep) {

		verts.clear();
		indices.clear();

		std::vector<glm::vec3> Spline1 = l1.BSpline(sprecision);
		std::vector<glm::vec3> Spline2 = l2.BSpline(sprecision);

		orderlines(Spline1, Spline2);

		for (int i = 0; i <= sprecision; i++) {
			glm::vec3 cvert = 0.5f * Spline1[i] + 0.5f * Spline2[i];
			glm::vec3 diameter = Spline1[i] - Spline2[i];

			float scale = 0.5 * glm::length(diameter);
			float theta = glm::acos(glm::dot(glm::normalize(diameter), glm::normalize(glm::vec3{ 0.f, 1.f, 0.f })));

			glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3{ 0, scale, scale });
			glm::mat4 R = glm::rotate(glm::mat4(1.f), theta, glm::vec3{ 0.f, 0.f, 1.f });
			glm::mat4 T = glm::translate(glm::mat4(1.f), cvert);

			for (auto j = sweep.begin(); j < sweep.end(); j++) {
				glm::vec3 point = T * R * S * glm::vec4(*j, 1.f);
				glm::vec3 normal = glm::normalize(point - cvert);
				verts.emplace_back(Vertex{ glm::vec4(point, 1.f), col, normal });
			}
		}

		// creating faces using vertex indices
		for (int i = 1; i <= sweep.size(); i++) {
			for (int j = 0; j <= sprecision; j++) {
				if (i != sweep.size()) {
					if (j != 0) {
						indices.push_back((sweep.size()) * j + i);
						indices.push_back((sweep.size()) * (j - 1) + i);
						indices.push_back((sweep.size()) * j + i - 1);
					}
					if (j != sprecision) {
						indices.push_back((sweep.size()) * j + i);
						indices.push_back((sweep.size()) * j + i - 1);
						indices.push_back((sweep.size()) * (j + 1) + i - 1);
					}
				}
				else {
					if (j != 0) {
						indices.push_back((sweep.size()) * (j) + 0);
						indices.push_back((sweep.size()) * (j - 1) + 0);
						indices.push_back((sweep.size()) * (j) + sweep.size() - 1);
					}
					if (j != sprecision) {
						indices.push_back((sweep.size()) * j + 0);
						indices.push_back((sweep.size()) * j + sweep.size() - 1);
						indices.push_back((sweep.size()) * (j + 1) + sweep.size() - 1);
					}
				}
			}
		}
	}

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