#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <vector>
#include <memory>
#include <string>
#include <iostream>

#include "Geometry.h"
#include "ShaderProgram.h"

int closestindex(std::vector<Vertex> points, glm::vec3 point, glm::vec3 ref) {
	int closest = -1;
	float min = 10.f;

	for (int i = 0; i < points.size(); i++) {
		float distance = glm::distance(ref * points[i].position, ref * point);
		if (distance < min) {
			min = distance;
			closest = i;
		}
	}
	return closest;
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
	glm::vec3 col;

	void draw() {
		geometry.bind();
		glDrawArrays(GL_LINE_STRIP, 0, GLsizei(verts.size()));
		glBindVertexArray(0);
	}

	void drawPoints(float pointSize) {
		geometry.bind();
		glPointSize(pointSize);
		glDrawArrays(GL_POINTS, 0, GLsizei(verts.size()));
		glBindVertexArray(0);
	}

	void ChaikinAlg(int iter) {
		glm::vec3 newpoint;
		std::vector<Vertex> Chaikin;
		for (int n = 0; n < iter; n++) {
			
			Chaikin.clear();
			int m = verts.size()-1;

			// C[0] = F[0]
			newpoint = verts[0].position;
			Chaikin.push_back(Vertex{newpoint, col, glm::vec3(0.f)});
			
			// C[1] = - 1/2F[0] + F[1] + 3/4F[2] - 1/4F[3]
			newpoint = -0.5f * verts[0].position + verts[1].position + 0.75f * verts[2].position - 0.25f * verts[3].position;
			Chaikin.push_back(Vertex{newpoint, col, glm::vec3(0.f) });

			for (int i = 2; i <= m-5; i += 2) {
				// C[j] = - 1/4F[i] + 3/4F[i+1] + 3/4F[i+2] - 1/4F[i+3]
				newpoint = -0.25f * verts[i].position + 0.75f * verts[i+1].position + 0.75f * verts[i+2].position - 0.25f * verts[i+3].position;
				Chaikin.push_back(Vertex{newpoint, col, glm::vec3(0.f)});
			}

			// C[j] = - 1/4F[m-3] + 3/4F[m-2] + F[m-1] - 1/2F[m]
			newpoint = -0.25f * verts[m - 3].position + 0.75f * verts[m - 2].position + verts[m - 1].position - 0.5f * verts[m].position;
			Chaikin.push_back(Vertex{ newpoint, col, glm::vec3(0.f) });

			// C[j+1] = F[m]
			newpoint = verts[m].position;
			Chaikin.push_back(Vertex{ newpoint, col, glm::vec3(0.f) });

			verts = Chaikin;
		}
	}

	void setColor(glm::vec3 mycolor) {
		for (auto i = verts.begin(); i < verts.end(); i++) {
			(*i).color = mycolor;
		}
		col = mycolor;
	}

	void BSpline(int precision, glm::vec3 color) {
		col = color;
		std::vector <Vertex> spline;
		
		float u;
		std::vector<float> basis = getbasis(3, verts.size() - 1);

		for (int i = 0; i <= precision; i++) {
			u = double(i) / precision;
			spline.push_back(Vertex{ getvert(verts, basis, u, 3, verts.size() - 1), col, glm::vec3(0.f, 0.f, 0.f) });
		}

		verts = spline;
	}

	void getCrossSection(Camera current, glm::vec3 fixed) {
		glm::vec3 p1 = verts[0].position;
		glm::vec3 p2 = verts.back().position;

		glm::vec3 scalevec = -1.f * (fixed - current.getUp()) + 1.f * current.getUp();

		glm::vec3 center = 0.5f * (p1 + p2);
		glm::vec3 d = p2 - p1;
		float dtheta = glm::orientedAngle(glm::normalize(current.getUp()), glm::normalize(d), -glm::normalize(current.getPos()));

		glm::mat4 T1 = glm::translate(glm::mat4(1.f), -center);
		glm::mat4 R1 = glm::rotate(glm::mat4(1.f), -dtheta, -current.getPos());
		glm::mat4 S1 = glm::scale(glm::mat4(1.f), scalevec);
		glm::mat4 S2 = glm::scale(glm::mat4(1.f), glm::vec3(2 / glm::length(d), 2 / glm::length(d), 2/ glm::length(d)) * fixed);

		std::vector<Vertex> temp = verts;
		verts.clear();
		for (int i = 0; i < temp.size() - 1; i++) {
			glm::vec3 newp = (S2 * R1 * T1 * glm::vec4(temp[i].position, 1.f));
			verts.push_back(Vertex{ newp, col, glm::vec3(0.f, 0.f, 0.f) });
		}
		for (int j = temp.size() - 1; j > 0; j--) {
			glm::vec3 newp2 = (S2 * S1 * R1 * T1 * glm::vec4(temp[j].position, 1.f));
			verts.push_back(Vertex{ newp2, col, glm::vec3(0.f, 0.f, 0.f) });
		}
	}

	void updateGPU() {
		geometry.bind();
		geometry.setVerts(verts);
	}

	Line(std::vector<Vertex> v)
		: verts(v)
		, standardized(false)
		, col(0,0,0)
	{}

	Line()
		: verts()
		, standardized(false)
		, col(0,0,0)
	{}
};