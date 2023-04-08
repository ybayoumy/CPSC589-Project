#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <vector>
#include <memory>
#include <string>
#include <iostream>

#include "Geometry.h"
#include "ShaderProgram.h"
#include "Line.h"
#include "Camera.h"

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
	std::vector<Vertex> Spline1 = l1.BSpline(sprecision, glm::vec3(0.f, 0.f, 0.f));
	std::vector<Vertex> Spline2 = l2.BSpline(sprecision, glm::vec3(0.f, 0.f, 0.f));

	orderlines(Spline1, Spline2);

	for (int i = 0; i <= sprecision; i++) {
		glm::vec3 cvert = 0.5f * Spline1[i].position + 0.5f * Spline2[i].position;
		axis.push_back(Vertex{ cvert, glm::vec3(1.f, 0.7f, 0.f), glm::vec3(0.f) });
	}

	return axis;
}

void indexGeometry(std::vector<unsigned int> &indices, int sweepsize, int sprecision) {

	indices.clear();
	
	for (int i = 1; i <= sweepsize; i++) {
		if (i == sweepsize) {
			indices.push_back(0 + 1);
			indices.push_back(0);
			indices.push_back(i);
		}
		else {
			indices.push_back(i + 1);
			indices.push_back(0);
			indices.push_back(i);
		}
	}

	// creating faces using vertex indices
	for (int i = 1; i <= sweepsize; i++) {
		for (int j = 0; j <= sprecision; j++) {
			if (i != sweepsize) {
				if (j != 0) {
					indices.push_back((sweepsize) * j + i + 1);
					indices.push_back((sweepsize) * (j - 1) + i + 1);
					indices.push_back((sweepsize) * j + i - 1 + 1);
				}
				if (j != sprecision) {
					indices.push_back((sweepsize) * j + i + 1);
					indices.push_back((sweepsize) * j + i - 1 + 1);
					indices.push_back((sweepsize) * (j + 1) + i - 1 + 1);
				}
			}
			else {
				if (j != 0) {
					indices.push_back((sweepsize) * (j)+0 + 1);
					indices.push_back((sweepsize) * (j - 1) + 0 + 1);
					indices.push_back((sweepsize) * (j)+sweepsize - 1 + 1);
				}
				if (j != sprecision) {
					indices.push_back((sweepsize) * j + 0 + 1);
					indices.push_back((sweepsize) * j + sweepsize - 1 + 1);
					indices.push_back((sweepsize) * (j + 1) + sweepsize - 1 + 1);
				}
			}
		}
	}

	for (int i = 1; i <= sweepsize; i++) {
		indices.push_back(sweepsize * (sprecision + 1) - (i - 1));
		indices.push_back(sweepsize * (sprecision + 1) + 1);
		indices.push_back(sweepsize * (sprecision + 1) - (i - 2));
		if (i == sweepsize) {
			indices.push_back(sweepsize * (sprecision + 1) - 0);
			indices.push_back(sweepsize * (sprecision + 1) + 1);
			indices.push_back(sweepsize * (sprecision + 1) - (i - 1));
		}
	}
}

class Mesh
{
public:

	std::vector<Vertex> verts;
	std::vector<unsigned int> indices;
	std::vector<Line> discs;

	Line bound1;
	Line bound2;

	Line sweep;

	std::vector<int> sweepindex;

	Line axis;

	glm::vec3 mycolor;

	//Line pinch1;
	//Line pinch2;

	//glm::vec3 direction;
	//glm::vec3 updirection;

	Camera cam;

	GPU_Geometry geometry;

	void create(int sprecision, glm::vec3 color) {
		verts.clear();
		indices.clear();

		mycolor = color;
	
		std::vector<Vertex> asweep;
		std::vector<Vertex> Spline1 = bound1.verts;
		std::vector<Vertex> Spline2 = bound2.verts;

		glm::vec3 cvert = glm::vec3(0.f);
		glm::vec3 diameter = glm::vec3(0.f);
		float scale;
		float theta;

		orderlines(Spline1, Spline2);

		//if (pinch1.verts.size() > 0 && pinch2.verts.size() > 0){
		//	std::vector<Vertex> PSpline1 = pinch1.BSpline(sprecision);
		//	std::vector<Vertex> PSpline2 = pinch2.BSpline(sprecision);

		//	orderlines(PSpline1, PSpline2);

		//	glm::vec3 eye = cam.getPos();
		//	glm::vec3 at = glm::vec3(0.f, 0.f, 0.f);

		//	glm::mat4 UnRot = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, cam.radius)) * glm::lookAt(eye, at, glm::vec3(0.f, 1.f, 0.f)) * glm::rotate(glm::mat4(1.f), -float(M_PI_2), cam.getUp());

		//	for (int i = 0; i <= sprecision; i++) {
		//		glm::vec3 cvert = axis[i].position;
		//		glm::vec3 diameter = Spline1[i].position - Spline2[i].position;
		//		glm::vec3 xdiameter = PSpline1[i].position - PSpline2[i].position;

		//		float xscale = 0.5 * glm::length(xdiameter);
		//		float scale = 0.5 * glm::length(diameter);

		//		float pratio = xscale / scale;

		//		float theta = glm::acos(glm::dot(glm::normalize(diameter), glm::normalize(cam.getUp())));

		//		glm::mat4 PScale = glm::scale(glm::mat4(1.f), glm::vec3(pratio, 1, pratio));

		//		glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3{scale, scale, scale});
		//		glm::mat4 R = glm::rotate(glm::mat4(1.f), theta, cam.getPos());
		//		glm::mat4 T = glm::translate(glm::mat4(1.f), cvert);

		//		for (auto j = sweep.verts.begin(); j < sweep.verts.end(); j++) {
		//			//glm::vec3 point = T * R * S * glm::inverse(UnRot) * PScale * 
		//			// obtain standardized sweep
		//			glm::vec3 point = UnRot * glm::vec4((*j).position, 1.f);
		//			// return to position, move to rotational blending surface
		//			point = T * R * S * glm::inverse(UnRot) * PScale * glm::vec4(point, 1.f);

		//			//std::cout << point << std::endl;

		//			glm::vec3 normal = glm::normalize(point - cvert);
		//			verts.emplace_back(Vertex{ glm::vec4(point, 1.f), color, normal });
		//		}
		//	}
		//}
		//else {

		for (int i = 0; i <= sprecision; i++) {

			asweep.clear();

			cvert = 0.5f * (Spline1[i].position + Spline2[i].position);
			axis.verts.emplace_back(Vertex{cvert, glm::vec3(1.f) - color, glm::vec3(0.f)});

			if (i == 0) {
				glm::vec3 cvertnext = 0.5f * Spline1[i+1].position + 0.5f * Spline2[i+1].position;
				verts.emplace_back(Vertex{ glm::vec4(cvert, 1.f), color, glm::normalize(cvert - cvertnext)});
			}

			diameter = (Spline1[i].position - Spline2[i].position);
			scale = 0.5 * glm::length(diameter);
			theta = glm::orientedAngle(glm::normalize(cam.getUp()), glm::normalize(diameter), -glm::normalize(cam.getPos()));

			glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3{ scale, scale, scale });
			glm::mat4 R = glm::rotate(glm::mat4(1.f), -theta, cam.getPos());
			glm::mat4 T = glm::translate(glm::mat4(1.f), cvert);
		
			for (int j = 0; j < sweep.verts.size(); j++) {
				glm::vec3 point = T * R * S * glm::vec4(sweep.verts[j].position, 1.f);
				glm::vec3 normal = glm::normalize(point - cvert);
				verts.emplace_back(Vertex{ glm::vec4(point, 1.f), color, normal });
				asweep.emplace_back(Vertex{ glm::vec4(point, 1.f), glm::vec3(1.f) - color, normal });
			}

			discs.emplace_back(Line(asweep));

			if (i == sprecision) {
				glm::vec3 cvertprev = 0.5f * Spline1[i - 1].position + 0.5f * Spline2[i - 1].position;
				verts.emplace_back(Vertex{ glm::vec4(cvert, 1.f), color, glm::normalize(cvert - cvertprev)});
			}
		
		}

		indexGeometry(indices, sweep.verts.size(), sprecision);
	}

	//std::vector<Vertex> getdisc(glm::vec3 position1, glm::vec3 position2, )

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
		for (auto i = verts.begin(); i < verts.end(); i++) {
			(*i).color = col;
		}
		mycolor = col;
	}

	Mesh(std::vector<Vertex>& v, std::vector<unsigned int>& i, Camera& c)
		: verts(v)
		, indices(i)
		, bound1()
		, bound2()
		, axis()
		, sweep()
		, sweepindex({0,0,0,0})
		, mycolor (0,0,0)
		//, pinch1()
		//, pinch2()
		, cam(c)
	{}

	Mesh()
		: verts()
		, indices()
		, bound1()
		, bound2()
		, axis()
		, sweep()
		, sweepindex({ 0,0,0,0 })
		, mycolor(0, 0, 0)
		//, pinch1()
		//, pinch2()
		, cam(0, 0, 1)
	{}
};