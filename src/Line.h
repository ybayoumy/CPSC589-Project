#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include <iostream>

#include "Geometry.h"
#include "ShaderProgram.h"

bool xdescend(Vertex vec1, Vertex vec2) {
	return (vec1.position.x < vec2.position.x);
}

bool ydescend(Vertex vec1, Vertex vec2) {
	return (vec1.position.y < vec2.position.y);
}

bool zdescend(Vertex vec1, Vertex vec2) {
	return (vec1.position.z < vec2.position.z);
}

std::vector <float> getbasis(int k, int m) {
	std::vector <float> basis;
	for (int i = 1; i <= 3; i++) {
		if (i == 1) {
			for (int j = 1; j <= k; j++) {
				basis.push_back(0);
			}
		}
		else if (i == 2) {
			for (int j = 1; j < m - k + 2; j++) {
				float inc = j / double(m - k + 2);
				basis.push_back(inc);
			}
		}
		else {
			for (int j = 1; j <= k; j++) {
				basis.push_back(1);
			}
		}
	}
	return basis;
}

// Algorithm to find delta (from A2 and Lecture)
int delta(std::vector <float> U, float u, int k, int m) {
	for (int i = 0; i <= m + k - 1; i++) {
		if (u >= U[i] && u < U[i + 1]) {
			return i;
		}
	}
	return -1;
}

// Efficient algorithm to find a value of the B-Spline at a given u value (from A2 and Lecture)
glm::vec3 getvert(std::vector<Vertex> E, std::vector <float> U, float u, int k, int m) {

	float omega;
	float denom;
	int i;
	int d = delta(U, u, k, m);

	std::vector <glm::vec3> C(k);

	if (d == -1) {
		d = m;
	}

	for (i = 0; i <= k - 1; i++) {
		C[i] = E[d - i].position;
	}


	for (int r = k; r >= 2; r--) {
		i = d;
		for (int s = 0; s <= r - 2; s++) {
			denom = U[i + r - 1] - U[i];
			if (denom != 0) {
				omega = (u - U[i]) / denom;
			}
			else omega = 0;

			C[s] = omega * C[s] + (1 - omega) * C[s + 1];
			i = i - 1;
		}
	}

	return C[0];
}

class Line
{
public:
	std::vector<Vertex> verts;
	GPU_Geometry geometry;
	bool standardized;

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

	std::vector<Vertex> BSpline(int precision) {
		std::vector <Vertex> spline;
		
		float u;
		std::vector<float> basis = getbasis(3, verts.size() - 1);


		for (int i = 0; i <= precision; i++) {
			u = double(i) / precision;
			spline.push_back(Vertex{ getvert(verts, basis, u, 3, verts.size() - 1), glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 0.f, 0.f) });
		}

		return spline;
	}

	void standardizesweep(glm::vec3 up, glm::vec3 view, glm::vec3 col) {
		std::vector<Vertex> temp;
		temp = verts;

		std::sort(temp.begin(), temp.end(), xdescend);

		glm::vec3 xmax = temp[0].position;
		glm::vec3 xmin = temp.back().position;

		std::sort(temp.begin(), temp.end(), ydescend);

		glm::vec3 ymax = temp[0].position;
		glm::vec3 ymin = temp.back().position;

		std::sort(temp.begin(), temp.end(), zdescend);

		glm::vec3 zmax = temp[0].position;
		glm::vec3 zmin = temp.back().position;

		glm::vec3 center;
		if (glm::vec3(0.f, 1.f, 0.f).y != 0) {
			center = 0.5f * (ymin + ymax);
		}

		float yd = fabs(ymax.y - ymin.y);
		
		//glm::vec3 recenter = glm::vec3(0.f, 0.f, 0.5f * (zmin.z + zmax.z));
		
		//float yd = glm::length(diameter);

		//float dtheta = glm::acos(glm::dot(glm::normalize(diameter), glm::normalize(up)));

		//glm::mat4 R = glm::rotate(glm::mat4(1.f), dtheta, view);

		temp = verts;
		verts.clear();
		for (int j = 0; j < temp.size(); j++) {
			glm::vec3 newvert = glm::scale(glm::mat4(1.f), glm::vec3(2 / yd, 2 / yd, 2 / yd)) * glm::translate(glm::mat4(1.f), -center) * glm::vec4(temp[j].position, 1.f);
			verts.push_back(Vertex{ newvert, col, glm::vec3(0.f, 0.f, 0.f) });
		}
	}

	void updateGPU() {
		geometry.bind();
		geometry.setVerts(verts);
	}

	Line(std::vector<Vertex> v)
		: verts(v)
		, standardized(false)
	{}

	Line()
		: verts()
		, standardized(false)
	{}
};