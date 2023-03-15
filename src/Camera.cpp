#include "Camera.h"
#include "Geometry.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include <iostream>

#include "glm/gtc/matrix_transform.hpp"


bool xdescend(Vertex vec1, Vertex vec2) {
	return (vec1.position.x < vec2.position.x);
}

bool ydescend(Vertex vec1, Vertex vec2) {
	return (vec1.position.y < vec2.position.y);
}

bool zdescend(Vertex vec1, Vertex vec2) {
	return (vec1.position.z < vec2.position.z);
}

Camera::Camera(float t, float p, float r) : theta(t), phi(p), radius(r) {
}

glm::mat4 Camera::getView() {
	glm::vec3 eye = radius * glm::vec3(std::cos(theta) * std::sin(phi), std::sin(theta), std::cos(theta) * std::cos(phi));
	glm::vec3 at = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

	return glm::lookAt(eye, at, up);
}

glm::vec3 Camera::getPos() {
	return radius * glm::vec3(std::cos(theta) * std::sin(phi), std::sin(theta), std::cos(theta) * std::cos(phi));
}

glm::vec3 Camera::getUp() {
	return glm::normalize(glm::vec3(-std::sin(phi) * std::sin(theta), std::cos(theta), -std::cos(phi) * std::sin(theta)));
}

void Camera::incrementTheta(float dt) {
	if (isFixed) return;

	if (theta + (dt / 100.0f) < M_PI_2 && theta + (dt / 100.0f) > -M_PI_2) {
		theta += dt / 100.0f;
	}
}

void Camera::incrementPhi(float dp) {
	if (isFixed) return;

	phi -= dp / 100.0f;
	if (phi > 2.0 * M_PI) {
		phi -= 2.0 * M_PI;
	} else if (phi < 0.0f) {
		phi += 2.0 * M_PI;
	}
}

void Camera::incrementR(float dr) {
	if (isFixed) return;

	radius -= dr;
}

std::vector<Vertex> Camera::getcircle(int inc) {
	glm::vec3 eye = radius * glm::vec3(std::cos(theta) * std::sin(phi), std::sin(theta), std::cos(theta) * std::cos(phi));
	glm::vec3 at = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 up = getUp();

	std::vector<Vertex> circle;
	for (int i = 0; i < inc; i++) {
		float angle = i * 2 * M_PI / inc;
		glm::vec3 point = glm::rotate(glm::mat4(1.f), -float(M_PI) / 2, up) * glm::inverse(glm::lookAt(eye, at, glm::vec3(0.f, 1.f, 0.f))) * glm::vec4(cos(angle), sin(angle), -radius, 1.f);
		circle.push_back(Vertex{point, glm::vec3(1.f, 0.7f, 0.f), glm::vec3(0.f, 0.f, 0.f) });
	}
	return circle;
}

std::vector<Vertex> Camera::standardize(std::vector<Vertex> line) {
	std::vector<Vertex> temp;
	temp = line;

	glm::vec3 eye = radius * glm::vec3(std::cos(theta) * std::sin(phi), std::sin(theta), std::cos(theta) * std::cos(phi));
	glm::vec3 at = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 up = getUp();

	line.clear();
	for (int j = 0; j < temp.size(); j++) {
		// GET POINTS INTO XY PLANE
		glm::vec3 newvert = getView() * glm::vec4(temp[j].position, 1.f);
		line.push_back(Vertex{ newvert, glm::vec3(1.f, 0.7f, 0.f), glm::vec3(0.f, 0.f, 0.f) });
	}

	std::sort(temp.begin(), temp.end(), xdescend);

	glm::vec3 xmax = temp[0].position;
	glm::vec3 xmin = temp.back().position;

	std::sort(temp.begin(), temp.end(), ydescend);

	glm::vec3 ymax = temp[0].position;
	glm::vec3 ymin = temp.back().position;

	glm::vec3 center;
	if (glm::vec3(0.f, 1.f, 0.f).y != 0) {
		center = 0.5f * (ymin + ymax);
	}

	float yd = fabs(ymax.y - ymin.y);

	temp = line;
	line.clear();
	for (int j = 0; j < temp.size(); j++) {
		// MOVE AND SCALE
		glm::vec3 newvert = glm::scale(glm::mat4(1.f), glm::vec3(2 / yd, 2 / yd, 0)) * glm::translate(glm::mat4(1.f), -center) * glm::vec4(temp[j].position, 1.f);
		// PUSH TO SCREEN
		newvert = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -radius)) * glm::vec4(newvert, 1.f);
		// ROTATE W.R.T AXIS
		newvert = glm::rotate(glm::mat4(1.f), float(M_PI_2), up) * glm::inverse(glm::lookAt(eye, at, glm::vec3(0.f, 1.f, 0.f))) * glm::vec4(newvert, 1.f);
		line.push_back(Vertex{ newvert, glm::vec3(1.f, 0.7f, 0.f), glm::vec3(0.f, 0.f, 0.f) });
	}

	return line;
}