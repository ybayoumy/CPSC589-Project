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

	float height;
	float width;

	glm::vec3 color;

	Line crosssection;
	Line sweep;

	Line ctrlpts1;
	Line ctrlpts2;

	Line pinch1;
	Line pinch2;

	//glm::vec3 direction;
	//glm::vec3 updirection;

	Camera cam;

	GPU_Geometry geometry;

	std::vector<Vertex> stdgetdisc(glm::vec3 cvert, glm::vec3 diameter, float theta) {
		std::vector<Vertex> disc;

		float scale = 0.5 * glm::length(diameter);
		
		glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3{ scale, scale, scale });
		glm::mat4 R = glm::rotate(glm::mat4(1.f), theta, -cam.getPos());
		glm::mat4 T = glm::translate(glm::mat4(1.f), cvert);

		for (int j = 0; j < sweep.verts.size(); j++) {
			glm::vec3 point = T * R * S * glm::vec4(sweep.verts[j].position, 1.f);
			disc.emplace_back(Vertex{ glm::vec4(point, 1.f), color, glm::vec3(0.f)});
		}

		return disc;
	}


	glm::vec3 getAxis() {
		glm::vec3 avgaxis = axis.back().position - axis[0].position;
		return glm::normalize(avgaxis);
	}

	glm::vec3 getCenter() {
		glm::vec3 center = 0.5f * axis.back().position + 0.5f * axis[0].position;
		return center;
	}

	glm::vec3 getPoint(glm::vec3 fix) {
		glm::vec3 y = fix * axis[0].position;
		glm::vec3 m = fix * getAxis();

		float t = (-(y.x + y.y + y.z)) / (m.x + m.y + m.z);
		glm::vec3 point = getAxis() * t + axis[0].position;
		return point;
	}

	Mesh gettempmesh() {
		Mesh tempmesh;
		tempmesh.ctrlpts1 = Line(ctrlpts1.verts);
		tempmesh.ctrlpts2 = Line(ctrlpts2.verts);
		tempmesh.sweep = Line(sweep.verts);
		tempmesh.pinch1 = Line(tempmesh.pinch1.verts);
		tempmesh.pinch2 = Line(tempmesh.pinch2.verts);
		tempmesh.cam = cam;
		tempmesh.color = color;

		glm::vec3 axis = getAxis();

		// fix angle
		// first isolate to direction of up
		glm::vec3 testup = axis * cam.getUp();
		if ((testup.x + testup.y + testup.z) < 0) {
			axis = axis * (glm::vec3(-1.f));
		}
		float profiletheta = glm::orientedAngle(cam.getUp(), axis, -cam.getPos());

		for (auto j = tempmesh.ctrlpts1.verts.begin(); j < tempmesh.ctrlpts1.verts.end(); j++) {
			(*j).position = glm::rotate(glm::mat4(1.f), -profiletheta, -cam.getPos()) * glm::translate(glm::mat4(1.f), -getCenter()) * glm::vec4((*j).position, 1.f);
		}

		for (auto i = tempmesh.ctrlpts2.verts.begin(); i < tempmesh.ctrlpts2.verts.end(); i++) {
			(*i).position = glm::rotate(glm::mat4(1.f), -profiletheta, -cam.getPos()) * glm::translate(glm::mat4(1.f), -getCenter()) * glm::vec4((*i).position, 1.f);
		}

		return tempmesh;
	}

	void create(int sprecision) {
		discs.clear();
		verts.clear();
		indices.clear();
		axis.clear();

		Line bound1 = Line(ctrlpts1.verts);
		Line bound2 = Line(ctrlpts2.verts);

		bound1.BSpline(sprecision, glm::vec3(0.f));
		bound2.BSpline(sprecision, glm::vec3(0.f));

		std::vector<Vertex> Spline1 = bound1.verts;
		std::vector<Vertex> Spline2 = bound2.verts;

		Line temppinch1;
		Line temppinch2;

		temppinch1 = Line(pinch1.verts);
		temppinch2 = Line(pinch2.verts);

		if (temppinch1.verts.size() > 0 && temppinch2.verts.size() > 0) {
			temppinch1.BSpline(2 * sprecision, glm::vec3(0.f, 0.f, 0.f));
			temppinch2.BSpline(2 * sprecision, glm::vec3(0.f, 0.f, 0.f));
		}

		glm::vec3 cvert = glm::vec3(0.f);
		glm::vec3 diameter = glm::vec3(0.f);
		glm::vec3 pdiameter = glm::vec3(0.f);

		float pscale;
		float scale;
		float theta;

		height = glm::distance(sweep.verts[0].position,sweep.verts[floor(sweep.verts.size() / 2)].position);
		width = glm::distance(sweep.verts[floor(1 * sweep.verts.size() / 4)].position, sweep.verts[floor(3 * sweep.verts.size() / 4)].position);

		orderlines(Spline1, Spline2);
		std::vector<Vertex> disc;

		for (int i = 0; i <= sprecision; i++) {

			cvert = 0.5f * (Spline1[i].position + Spline2[i].position);
			axis.push_back(Vertex{ glm::vec4(cvert, 1.f), color, glm::vec3(0.f, 0.f, 0.f) });

			if (i == 0) {
				glm::vec3 cvertnext = 0.5f * Spline1[i+1].position + 0.5f * Spline2[i+1].position;
				verts.emplace_back(Vertex{ glm::vec4(cvert, 1.f), color, glm::normalize(cvert - cvertnext)});
			}

			diameter = (Spline1[i].position - Spline2[i].position);
			scale = (1.f / height) * glm::length(diameter);
			theta = glm::orientedAngle(glm::normalize(cam.getUp()), glm::normalize(diameter), -glm::normalize(cam.getPos()));

			if (temppinch1.verts.size() > 0 && temppinch2.verts.size() > 0) {
				glm::vec3 P1 = closestvec(temppinch1.verts, cvert, cam.getUp());
				glm::vec3 P2 = closestvec(temppinch2.verts, cvert, cam.getUp());

				pdiameter = P2 - P1;

				cvert = cvert * (glm::vec3(1.f) - glm::abs(glm::normalize(cam.getPos()))) + (0.5f * (P1 + P2) * glm::abs(glm::normalize(cam.getPos())));

				pscale = (1.f / width) * glm::length(pdiameter);

				glm::vec3 scaleby = pscale * glm::abs(glm::normalize(cam.getPos())) + scale * (glm::vec3(1.f) - glm::abs(glm::normalize(cam.getPos())));
			
				glm::mat4 S = glm::scale(glm::mat4(1.f), scaleby);
				glm::mat4 R = glm::rotate(glm::mat4(1.f), theta, -cam.getPos());
				glm::mat4 T = glm::translate(glm::mat4(1.f), cvert);

				disc.clear();
				for (int j = 0; j < sweep.verts.size(); j++) {
					glm::vec3 point = T * R * S * glm::vec4(sweep.verts[j].position, 1.f);

					glm::vec3 normal = glm::vec3(0.f);

					verts.emplace_back(Vertex{ glm::vec4(point, 1.f), color, glm::vec3(0.f) });
					disc.push_back(verts.back());
				}

				discs.push_back(disc);
			}

			else {
				disc.clear();
				disc = stdgetdisc(cvert, diameter, theta);
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

		// NORMALS CALCULATION

		glm::vec3 nextONring;
		glm::vec3 nextring;
		for (int i = 1; i < (verts.size()-1); i++) {
			if (i % sweep.verts.size() == 0) {
				if (i <= sweep.verts.size()) {
					nextONring = verts[i - (sweep.verts.size() - 1)].position - verts[i].position;
					nextring = verts[i + sweep.verts.size()].position - verts[i].position;
				}
				else {
					nextONring = verts[i - 1].position - verts[i].position;
					nextring = verts[i - sweep.verts.size()].position - verts[i].position;
				}
			}
			else {
				if (i <= sweep.verts.size()) {
					nextONring = verts[i + 1].position - verts[i].position;
					nextring = verts[i + sweep.verts.size()].position - verts[i].position;
				}
				else {
					nextONring = verts[i - 1].position - verts[i].position;
					nextring = verts[i - sweep.verts.size()].position - verts[i].position;
				}
			}
			verts[i].normal = glm::cross(nextONring, nextring);
		}

		updateindices(indices, sweep.verts.size(), sprecision);

		temppinch1.verts.clear();
		temppinch2.verts.clear();
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

		output1.ChaikinAlg(4);
		output2.ChaikinAlg(4);

		output.emplace_back(Line(output1.verts));
		output.emplace_back(Line(output2.verts));

		return output;
	}

	void setcrosssection(std::vector<Vertex> cross, glm::vec3 fixed, int precision) {
		crosssection = cross;

		Line temp;
		temp = Line(cross);
		temp.MakeCrossSection(cam, fixed);
		temp.MakeSweep(cam, fixed);
		cam.standardize(temp.verts);
		temp.BSpline(precision, color);

		sweep.verts = temp.verts;
	}

	Line getCrosssection(glm::vec3 p1, glm::vec3 p2, glm::vec3 fixed) {
		if (crosssection.verts.size() > 0) return crosssection.verts;
		else {
			glm::vec3 scalevec = -1.f * fixed + 2.f * glm::abs(glm::normalize(cam.getUp()));

			glm::vec3 center = 0.5f * (p1 + p2);
			glm::vec3 d = p2 - p1;

			// fix angle
			// first isolate to direction of up
			glm::vec3 testup = d * cam.getUp();
			if ((testup.x + testup.y + testup.z) < 0) {
				d = d * (glm::vec3(-1.f));
			}

			float dtheta = glm::orientedAngle(glm::normalize(cam.getUp()), glm::normalize(d), -glm::normalize(cam.getPos()));

			glm::mat4 T1 = glm::translate(glm::mat4(1.f), center);
			glm::mat4 R1 = glm::rotate(glm::mat4(1.f), dtheta, -cam.getPos());
			glm::mat4 S1 = glm::scale(glm::mat4(1.f), glm::vec3(2 / glm::length(d), 2 / glm::length(d), 2 / glm::length(d)));

			Line output;
			for (int i = floor(1 * (sweep.verts.size() + 1) / 4); i < floor(3 * (sweep.verts.size() + 1) / 4); i++) {
				output.verts.push_back(sweep.verts[i]);
			}
			cam.standardize(output.verts);
			for (auto i = output.verts.begin(); i < output.verts.end(); i++) {
				(*i).position = T1 * glm::inverse(S1) * R1 * glm::vec4((*i).position, 1.f);
			}
			return output;
		}
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
		, sweep()
		, width(2.f)
		, height(2.f)
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
		, sweep()
		, width(2.f)
		, height(2.f)
		//, pinch1()
		//, pinch2()
		, cam(0, 0, 1)
	{}
};