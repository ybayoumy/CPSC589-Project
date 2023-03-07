#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include <iostream>

#include "Geometry.h"
#include "ShaderProgram.h"
#include "Line.h"

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

		for (int i = 0; i <= sprecision; i++) {
			glm::vec3 cvert = 0.5f * Spline1[i] + 0.5f * Spline2[i];
			glm::vec3 diameter = Spline1[i] - Spline2[i];

			float scale = 0.5 * glm::length(diameter);
			float theta = glm::acos(glm::dot(glm::normalize(diameter), glm::normalize(glm::vec3{ 0.f, 1.f, 0.f })));

			glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3{ 0, scale, scale });
			glm::mat4 R = glm::rotate(glm::mat4(1.f), -theta, glm::vec3{ 0.f, 0.f, 1.f });
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
		}

		for (auto i = indices.begin(); i < indices.end(); i++) {
			std::cout << (*i) << std::endl;
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