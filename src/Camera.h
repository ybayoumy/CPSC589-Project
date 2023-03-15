#pragma once

//------------------------------------------------------------------------------
// This file contains an implementation of a spherical camera
//------------------------------------------------------------------------------

//#include <GL/glew.h>
#include <vector>
#include <Geometry.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Camera {
public:

	Camera(float t, float p, float r);

	glm::mat4 getView();
	glm::vec3 getPos();
	glm::vec3 getUp();
	std::vector<Vertex> getcircle(int inc);
	std::vector<Vertex> standardize(std::vector<Vertex>);

	void incrementTheta(float dt);
	void incrementPhi(float dp);
	void incrementR(float dr);

	void fix() { isFixed = true; }
	void unFix() { isFixed = false; }

	bool isFixed = false;

	float theta;
	float phi;
	float radius;
};
