#include "Camera.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <iostream>

#include "glm/gtc/matrix_transform.hpp"

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
	/*glm::vec3 eye = radius * glm::vec3(std::cos(theta) * std::sin(phi), std::sin(theta), std::cos(theta) * std::cos(phi));
	if (eye != glm::vec3(0.f, 0.f, 1.f)) return glm::cross(eye, glm::vec3(0.f, 0.f, 1.f));
	else*/
	return glm::vec3(0.f, 1.f, 0.f);
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

std::vector<glm::vec3> Camera::getcircle(int inc) {
	glm::vec3 eye = radius * glm::vec3(std::cos(theta) * std::sin(phi), std::sin(theta), std::cos(theta) * std::cos(phi));
	glm::vec3 at = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

	std::vector<glm::vec3> circle;
	for (int i = 0; i < inc; i++) {
		float angle = i * 2 * M_PI / inc;
		glm::vec3 point = glm::inverse(glm::lookAt(eye, at, up)) * glm::vec4(cos(angle), sin(angle), -radius, 1.f);
		circle.push_back(point);
	}
	return circle;
}