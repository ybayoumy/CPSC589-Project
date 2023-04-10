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

glm::vec3 closestvec(std::vector<Vertex> points, glm::vec3 point, glm::vec3 ref) {
	glm::vec3 closest = glm::vec3 (10.f, 10.f, 10.f);
	float min = 10.f;

	for (auto i = points.begin(); i < points.end(); i++) {
		float distance = glm::distance(ref * (*i).position, ref * point);
		if (distance < min) {
			closest = (*i).position;
			min = distance;
		}
	}
	return closest;
}

std::vector<Vertex> centeraxis(Line l1, Line l2, int sprecision) {
	l1.BSpline(sprecision, glm::vec3(0.f));
	l2.BSpline(sprecision, glm::vec3(0.f));

	std::vector<Vertex> axis;
	std::vector<Vertex> Spline1 = l1.verts;
	std::vector<Vertex> Spline2 = l2.verts;

	orderlines(Spline1, Spline2);

	for (int i = 0; i <= sprecision; i++) {
		glm::vec3 cvert = 0.5f * Spline1[i].position + 0.5f * Spline2[i].position;
		axis.push_back(Vertex{ cvert, glm::vec3(1.f, 0.7f, 0.f), glm::vec3(0.f) });
	}

	return axis;
}

void updateindices(std::vector<unsigned int> &indices, int sweepsize, int sprecision) {
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

	std::vector<Vertex> axis;
	std::vector<std::vector<Vertex>> discs;

	glm::vec3 color;

	Line crosssection;

	Line bound1;
	Line bound2;
	Line sweep;

	Line ctrlpts1;
	Line ctrlpts2;

	Line pinch1;
	Line pinch2;

	glm::vec3 fixed;
	glm::vec3 unfixed;
	glm::vec3 up;

	//glm::vec3 direction;
	//glm::vec3 updirection;

	Camera cam;

	GPU_Geometry geometry;

	std::vector<Vertex> stdgetdisc(glm::vec3 cvert, glm::vec3 diameter, float theta) {
		std::vector<Vertex> disc;

		float scale = 0.5 * glm::length(diameter);
		
		glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3{ scale, scale, scale });
		glm::mat4 R = glm::rotate(glm::mat4(1.f), -theta, cam.getPos());
		glm::mat4 T = glm::translate(glm::mat4(1.f), cvert);

		for (int j = 0; j < sweep.verts.size(); j++) {
			glm::vec3 point = T * R * S * glm::vec4(sweep.verts[j].position, 1.f);
			glm::vec3 normal = glm::normalize(point - cvert);
			disc.emplace_back(Vertex{ glm::vec4(point, 1.f), color, normal });
		}

		return disc;
	}

	void create(int sprecision) {
		discs.clear();
		verts.clear();
		indices.clear();
		axis.clear();

		bound1 = Line(ctrlpts1.verts);
		bound2 = Line(ctrlpts2.verts);

		bound1.BSpline(sprecision, glm::vec3(0.f));
		bound2.BSpline(sprecision, glm::vec3(0.f));

		std::vector<Vertex> Spline1 = bound1.verts;
		std::vector<Vertex> Spline2 = bound2.verts;

		Line temppinch1;
		Line temppinch2;

		temppinch1 = Line(pinch1.verts);
		temppinch2 = Line(pinch2.verts);

		if (temppinch1.verts.size() > 0 && temppinch2.verts.size() > 0) {
			temppinch1.BSpline(sprecision, glm::vec3(0.f, 0.f, 0.f));
			temppinch2.BSpline(sprecision, glm::vec3(0.f, 0.f, 0.f));
		}

		glm::vec3 cvert = glm::vec3(0.f);
		glm::vec3 diameter = glm::vec3(0.f);
		float scale;
		float theta;

		fixed = glm::vec3(1.f) - glm::normalize(glm::abs(cam.getPos()));
		unfixed = glm::normalize(glm::vec3(cam.getPos().x, cam.getPos().y, -cam.getPos().z));

		orderlines(Spline1, Spline2);

		for (int i = 0; i <= sprecision; i++) {

			cvert = 0.5f * (Spline1[i].position + Spline2[i].position);
			axis.push_back(Vertex{ glm::vec4(cvert, 1.f), color, glm::vec3(0.f, 0.f, 0.f) });

			if (i == 0) {
				glm::vec3 cvertnext = 0.5f * Spline1[i+1].position + 0.5f * Spline2[i+1].position;
				verts.emplace_back(Vertex{ glm::vec4(cvert, 1.f), color, glm::normalize(cvert - cvertnext)});
			}

			diameter = (Spline1[i].position - Spline2[i].position);
			scale = 0.5 * glm::length(diameter);
			theta = glm::orientedAngle(glm::normalize(cam.getUp()), glm::normalize(diameter), -glm::normalize(cam.getPos()));

			if (temppinch1.verts.size() > 0 && temppinch2.verts.size() > 0) {
				glm::vec3 P1 = closestvec(temppinch1.verts, cvert, up);
				glm::vec3 P2 = closestvec(temppinch2.verts, cvert, up);
				glm::vec3 pdiameter = P2 - P1;

				cvert = (cvert * fixed) + ((0.5f * (P1 + P2)) * glm::abs(unfixed));

				float pscale = 0.5 * glm::length(pdiameter);
				glm::vec3 scaleby = pscale * unfixed + scale * fixed;

				glm::mat4 S = glm::scale(glm::mat4(1.f), scaleby);
				glm::mat4 R = glm::rotate(glm::mat4(1.f), -theta, cam.getPos());
				glm::mat4 T = glm::translate(glm::mat4(1.f), cvert);

				for (int j = 0; j < sweep.verts.size(); j++) {
					glm::vec3 point = T * R * S * glm::vec4(sweep.verts[j].position, 1.f);
					glm::vec3 normal = glm::normalize(point - cvert);
					verts.emplace_back(Vertex{ glm::vec4(point, 1.f), color, normal });
				}
			}

			else {
				std::vector<Vertex> disc = stdgetdisc(cvert, diameter, theta);
				int k = 0;
				for (auto j = disc.begin(); j < disc.end(); j++) {
					verts.emplace_back((*j));
				}
				discs.push_back(disc);
			}

			if (i == sprecision) {
				glm::vec3 cvertprev = 0.5f * Spline1[i - 1].position + 0.5f * Spline2[i - 1].position;
				verts.emplace_back(Vertex{ glm::vec4(cvert, 1.f), color, glm::normalize(cvert - cvertprev)});
			}
		
		}

		updateindices(indices, sweep.verts.size(), sprecision);

		temppinch1.verts.clear();
		temppinch2.verts.clear();

	}

	glm::vec3 getAxis() {
		glm::vec3 avgaxis = axis.back().position - axis[0].position;
		return glm::normalize(avgaxis);
	}

	glm::vec3 getPoint(glm::vec3 fix) {
		glm::vec3 y = fix * axis[0].position;
		glm::vec3 m = fix * getAxis();

		float t = (-(y.x + y.y + y.z)) / (m.x + m.y + m.z);
		glm::vec3 point = getAxis() * t + axis[0].position;
		return point;
	}

	void setPinch(glm::vec3 current) {
		
		std::vector<Vertex> P1 = pinch1.verts;
		std::vector<Vertex> P2 = pinch2.verts;

		glm::vec3 nochange = fixed - glm::normalize(glm::abs(current));
		glm::vec3 drawaxis = getAxis();
		glm::vec3 axisstart = getPoint(nochange);

		orderlines(P1, P2);

		glm::vec3 center1 = 0.5f * (P1[0].position + P1.back().position);
		glm::vec3 center2 = 0.5f * (P2[0].position + P2.back().position);
		glm::vec3 newcenter = 0.5f * (axis[0].position + axis.back().position);

		for (auto i = P1.begin(); i < P1.end(); i++) {
			glm::vec3 y = nochange * (*i).position;
			glm::vec3 m = nochange * drawaxis;

			float t = (y.x + y.y + y.z) / (m.x + m.y + m.z);
			(*i).position = ((glm::vec3(1.f) - fixed) * (*i).position) + (fixed * (axisstart + drawaxis * t));
		}
		for (auto j = P2.begin(); j < P2.end(); j++) {
			glm::vec3 y = nochange * (*j).position;
			glm::vec3 m = nochange * drawaxis;

			float s = (y.x + y.y + y.z) / (m.x + m.y + m.z);
			(*j).position = ((glm::vec3(1.f) - fixed) * (*j).position) + (fixed * (axisstart + drawaxis * s));
		}

		pinch1.verts.clear();
		pinch2.verts.clear();
		pinch1 = Line(P1);
		pinch2 = Line(P2);

		up = nochange;
	}

	std::vector<Line> getPinches(int sprecision) {
		std::vector<Line> output;
		Line output1;
		Line output2;

		for (auto i = discs.begin(); i < discs.end(); i++){

			std::vector<Vertex> disc = (*i);
			output1.verts.push_back(disc[0]);
			output2.verts.push_back(disc[floor(sprecision/2)]);

		}

		output1.ChaikinAlg(2);
		output2.ChaikinAlg(2);

		output.emplace_back(Line(output1.verts));
		output.emplace_back(Line(output2.verts));

		return output;
	}

	void setcrosssection(std::vector<Vertex> cross, glm::vec3 fixed, int precision) {
		crosssection = cross;

		Line temp;
		temp = Line(cross);
		temp.getCrossSection(cam, fixed);
		cam.standardize(temp.verts);

		crosssection = temp.verts;
		temp.BSpline(precision, color);

		sweep = temp.verts;
	}

	void draw() {
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

		for (Vertex& v : verts) {
			v.color = color;
		}

		updateGPU();
	}

	Mesh(std::vector<Vertex>& v, std::vector<unsigned int>& i, Camera& c)
		: verts(v)
		, indices(i)
		, color(glm::vec3(0.f, 0.f, 0.f))
		, ctrlpts1()
		, ctrlpts2()
		, bound1()
		, bound2()
		, sweep()
		//, pinch1()
		//, pinch2()
		, cam(c)
	{}

	Mesh()
		: verts()
		, indices()
		, color(glm::vec3(0.f, 0.f, 0.f))
		, ctrlpts1()
		, ctrlpts2()
		, bound1()
		, bound2()
		, sweep()
		//, pinch1()
		//, pinch2()
		, cam(0, 0, 1)
	{}
};