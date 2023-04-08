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
	glm::vec4 getCursorPos(glm::vec2 mouseIn);
	glm::vec2 getMousePos(glm::vec4 cursorIn);

	glm::vec4 drawonplane(glm::vec2 mouseIn, glm::vec3 axis, glm::vec3 point, glm::vec3 fixed);

	std::vector<Vertex> getcircle(int inc);
	std::vector<Vertex> getthincircle(int inc);
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
