#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include <iostream>

#include "Geometry.h"
#include "ShaderProgram.h"
#include "Line.h"

void orderlines(std::vector<Vertex>& Line1, std::vector<Vertex>& Line2) {
	float dist1 = glm::distance(Line1[0].position, Line2[0].position);
	float dist2 = glm::distance(Line1[0].position, Line2[Line2.size() - 1].position);
	float dist3 = glm::distance(Line1[Line1.size() - 1].position, Line2[0].position);
	float dist4 = glm::distance(Line1[Line1.size() - 1].position, Line2[Line2.size() - 1].position);

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

std::vector<Vertex> centeraxis(Line l1, Line l2, int sprecision) {
	std::vector<Vertex> axis;
	std::vector<Vertex> Spline1 = l1.BSpline(sprecision);
	std::vector<Vertex> Spline2 = l2.BSpline(sprecision);

	orderlines(Spline1, Spline2);

	for (int i = 0; i <= sprecision; i++) {
		glm::vec3 cvert = 0.5f * Spline1[i].position + 0.5f * Spline2[i].position;
		axis.push_back(Vertex{ cvert, glm::vec3(1.f, 0.7f, 0.f), glm::vec3(0.f) });
	}

	return axis;
}

class Mesh
{
public:
	std::vector<Vertex> verts;
	std::vector<unsigned int> indices;
	glm::vec3 direction;
	glm::vec3 color;

	GPU_Geometry geometry;

	void create(Line l1, Line l2, int sprecision, Line sweep, Line pinch1, Line pinch2, glm::vec3 up, glm::vec3 view) {

		verts.clear();
		indices.clear();

		std::vector<Vertex> axis;
		std::vector<Vertex> Spline1 = l1.BSpline(sprecision);
		std::vector<Vertex> Spline2 = l2.BSpline(sprecision);

		orderlines(Spline1, Spline2);

		for (int i = 0; i <= sprecision; i++) {
			glm::vec3 cvert = 0.5f * Spline1[i].position + 0.5f * Spline2[i].position;
			axis.push_back(Vertex{ cvert, glm::vec3(1.f, 0.7f, 0.f), glm::vec3(0.f) });
		}
		
		if (pinch1.verts.size() > 0 && pinch2.verts.size() > 0){
			std::vector<Vertex> PSpline1 = pinch1.BSpline(sprecision);
			std::vector<Vertex> PSpline2 = pinch2.BSpline(sprecision);

			for (int i = 0; i <= sprecision; i++) {
				glm::vec3 cvert = axis[i].position;
				glm::vec3 diameter = Spline1[i].position - Spline2[i].position;
				glm::vec3 xdiameter = PSpline1[i].position - PSpline2[i].position;

				float xscale = 0.5 * glm::length(xdiameter);
				std::cout << xscale << std::endl;
				float scale = 0.5 * glm::length(diameter);
				float theta = glm::acos(glm::dot(glm::normalize(diameter), glm::normalize(up)));

				glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3{ xscale, scale, xscale });
				glm::mat4 R = glm::rotate(glm::mat4(1.f), theta, view);
				glm::mat4 T = glm::translate(glm::mat4(1.f), cvert);

				for (auto j = sweep.verts.begin(); j < sweep.verts.end(); j++) {
					glm::vec3 point = T * R * S * glm::vec4((*j).position, 1.f);
					glm::vec3 normal = glm::normalize(point - cvert);
					verts.emplace_back(Vertex{ glm::vec4(point, 1.f), color, normal });
				}
			}
		}
		else {
			for (int i = 0; i <= sprecision; i++) {
				glm::vec3 cvert = axis[i].position;
				glm::vec3 diameter = Spline1[i].position - Spline2[i].position;

				float scale = 0.5 * glm::length(diameter);
				float theta = glm::acos(glm::dot(glm::normalize(diameter), glm::normalize(up)));

				glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3{ scale, scale, scale });
				glm::mat4 R = glm::rotate(glm::mat4(1.f), theta, view);
				glm::mat4 T = glm::translate(glm::mat4(1.f), cvert);

				for (auto j = sweep.verts.begin(); j < sweep.verts.end(); j++) {
					glm::vec3 point = T * R * S * glm::vec4((*j).position, 1.f);
					glm::vec3 normal = glm::normalize(point - cvert);
					verts.emplace_back(Vertex{ glm::vec4(point, 1.f), color, normal });
				}
			}
		}
		
		// creating faces using vertex indices
		for (int i = 1; i <= sweep.verts.size(); i++) {
			for (int j = 0; j <= sprecision; j++) {
				if (i != sweep.verts.size()) {
					if (j != 0) {
						indices.push_back((sweep.verts.size()) * j + i);
						indices.push_back((sweep.verts.size()) * (j - 1) + i);
						indices.push_back((sweep.verts.size()) * j + i - 1);
					}
					if (j != sprecision) {
						indices.push_back((sweep.verts.size()) * j + i);
						indices.push_back((sweep.verts.size()) * j + i - 1);
						indices.push_back((sweep.verts.size()) * (j + 1) + i - 1);
					}
				}
				else {
					if (j != 0) {
						indices.push_back((sweep.verts.size()) * (j) + 0);
						indices.push_back((sweep.verts.size()) * (j - 1) + 0);
						indices.push_back((sweep.verts.size()) * (j) + sweep.verts.size() - 1);
					}
					if (j != sprecision) {
						indices.push_back((sweep.verts.size()) * j + 0);
						indices.push_back((sweep.verts.size()) * j + sweep.verts.size() - 1);
						indices.push_back((sweep.verts.size()) * (j + 1) + sweep.verts.size() - 1);
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

	void setColor(glm::vec3 col) {
		color = col;
	}

	Mesh(std::vector<Vertex>& v, std::vector<unsigned int>& i)
		: verts(v)
		, indices(i)
		, color(glm::vec3(1.f, 0.f, 0.f))
	{}

	Mesh()
		: verts()
		, indices()
		, color(glm::vec3(1.f, 0.f, 0.f))
	{}
};