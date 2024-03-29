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

glm::vec4 Camera::getCursorPos(glm::vec2 mouseIn) {
	float perspectiveMultiplier = glm::tan(glm::radians(22.5f)) * radius;
	glm::vec4 cursorPos = glm::vec4(mouseIn * perspectiveMultiplier, -radius, 1.0f);
	cursorPos = glm::inverse(getView()) * cursorPos;

	return cursorPos;
}

glm::vec4 Camera::getCursorPosOP(glm::vec2 mouseIn, glm::vec3 fixed, glm::vec3 nochange, glm::vec3 drawaxis, glm::vec3 axisstart) {
	
	glm::vec3 ref = fixed - nochange;

	glm::vec4 cursorPos = getCursorPos(mouseIn);

	glm::vec3 y = glm::vec4(nochange, 1.f) * cursorPos;
	glm::vec3 m = nochange * drawaxis;

	float t = (y.x + y.y + y.z) / (m.x + m.y + m.z);
	float disttoaxis = glm::distance(ref * getPos(), ref * (axisstart + drawaxis * t));

	cursorPos = glm::vec4(mouseIn * glm::tan(glm::radians(22.5f)) * disttoaxis, -disttoaxis, 1.0f);
	cursorPos = glm::inverse(getView()) * cursorPos;

	return cursorPos;
}

glm::vec2 Camera::getMousePos(glm::vec4 cursorIn) {
	glm::vec4 mousePos = getView() * cursorIn;
	float perspectiveMultiplier = glm::tan(glm::radians(22.5f)) * radius;
	mousePos = (1/perspectiveMultiplier) * mousePos;
	return mousePos;
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

	if (radius > 10.0f) radius = 10.0f;
	else if (radius < 0.2f) radius = 0.2f;
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

void Camera::standardize(std::vector<Vertex> &myverts) {
	std::vector<Vertex> temp;

	for (auto j = myverts.begin(); j < myverts.end(); j++) {
		// ROTATE W.R.T AXIS
		glm::vec3 newvert = glm::rotate(glm::mat4(1.f), float(M_PI_2), getUp()) * glm::vec4((*j).position, 1.f);
		temp.push_back(Vertex{ newvert, glm::vec3(1.f, 0.7f, 0.f), glm::vec3(0.f, 0.f, 0.f) });
	}

	myverts.clear();
	myverts = temp;
}