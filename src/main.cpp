#include <iostream>
#include <fstream>

#define _USE_MATH_DEFINES
#include <math.h>

#include <string>
#include <list>
#include <vector>
#include <limits>
#include <functional>
#include <unordered_map>

// Window.h `#include`s ImGui, GLFW, and glad in correct order.
#include "Window.h"

#include "Geometry.h"
#include "GLDebug.h"
#include "Log.h"
#include "ShaderProgram.h"
#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "Line.h"

#include "Renderbuffer.h"
#include "Framebuffer.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

// EXAMPLE CALLBACKS
class Callbacks3D : public CallbackInterface
{

public:
	// Constructor. We use values of -1 for attributes that, at the start of
	// the program, have no meaningful/"true" value.
	Callbacks3D(ShaderProgram &lightingShader, ShaderProgram &noLightingShader, ShaderProgram &pickerShader, Camera &camera, int screenWidth, int screenHeight)
		: lightingShader(lightingShader), noLightingShader(noLightingShader), pickerShader(pickerShader), camera(camera), rightMouseDown(false), leftMouseDown(false), mouseOldX(-1.0), mouseOldY(-1.0), screenWidth(screenWidth), screenHeight(screenHeight), aspect(screenWidth / screenHeight)
	{
		updateUniformLocations();
	}

	void updateIDUniform(int idToRender)
	{
		// Like viewPipelinePicker(), this function assumes pickerShader.use() was called before.
		glUniform1i(pickerIDLoc, idToRender);
	}

	virtual void keyCallback(int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_R && action == GLFW_PRESS)
		{
			lightingShader.recompile();
			noLightingShader.recompile();
			pickerShader.recompile();
			updateUniformLocations();
		}
	}

	virtual void mouseButtonCallback(int button, int action, int mods)
	{
		// If we click the mouse on the ImGui window, we don't want to log that
		// here. But if we RELEASE the mouse over the window, we do want to
		// know that!
		auto &io = ImGui::GetIO();
		if (io.WantCaptureMouse && action == GLFW_PRESS)
			return;

		if (button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			if (action == GLFW_PRESS) {
				rightMouseDown = true;
			}
			else if (action == GLFW_RELEASE)
				rightMouseDown = false;
		}

		if (button == GLFW_MOUSE_BUTTON_LEFT)
		{
			if (action == GLFW_PRESS) {
				leftMouseDown = true;
				lastLeftPressedFrame = currentFrame;
			}
			else if (action == GLFW_RELEASE)
				leftMouseDown = false;
		}
	}

	bool leftMouseJustPressed() {
		return lastLeftPressedFrame == currentFrame;
	}
	void incrementFrameCount() {
		currentFrame++;
	}

	// Updates the screen width and height, in screen coordinates
	// (not necessarily the same as pixels)
	virtual void windowSizeCallback(int width, int height)
	{
		screenWidth = width;
		screenHeight = height;
		aspect = float(width) / float(height);
	}

	virtual void cursorPosCallback(double xpos, double ypos)
	{
		if (rightMouseDown)
		{
			camera.incrementTheta(ypos - mouseOldY);
			camera.incrementPhi(xpos - mouseOldX);
		}
		mouseOldX = xpos;
		mouseOldY = ypos;
	}
	virtual void scrollCallback(double xoffset, double yoffset)
	{
		camera.incrementR(yoffset);
	}

	void lightingViewPipeline()
	{
		glm::mat4 M = glm::mat4(1.0);
		glm::mat4 V = camera.getView();
		glm::mat4 P = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 1000.f);
		glUniformMatrix4fv(lightingMLoc, 1, GL_FALSE, glm::value_ptr(M));
		glUniformMatrix4fv(lightingVLoc, 1, GL_FALSE, glm::value_ptr(V));
		glUniformMatrix4fv(lightingPLoc, 1, GL_FALSE, glm::value_ptr(P));
	}

	void noLightingViewPipeline()
	{
		glm::mat4 M = glm::mat4(1.0);
		glm::mat4 V = camera.getView();
		glm::mat4 P = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 1000.f);
		glUniformMatrix4fv(noLightingMLoc, 1, GL_FALSE, glm::value_ptr(M));
		glUniformMatrix4fv(noLightingVLoc, 1, GL_FALSE, glm::value_ptr(V));
		glUniformMatrix4fv(noLightingPLoc, 1, GL_FALSE, glm::value_ptr(P));
	}

	void viewPipelinePicker()
	{
		glm::mat4 M = glm::mat4(1.0);
		glm::mat4 V = camera.getView();
		glm::mat4 P = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 1000.f);

		glUniformMatrix4fv(mLocPicker, 1, GL_FALSE, glm::value_ptr(M));
		glUniformMatrix4fv(vLocPicker, 1, GL_FALSE, glm::value_ptr(V));
		glUniformMatrix4fv(pLocPicker, 1, GL_FALSE, glm::value_ptr(P));
	}

	void updateShadingUniforms(
		const glm::vec3 &lightPos, float diffuseConstant, float ambientStrength)
	{
		// Like viewPipeline(), this function assumes shader.use() was called before.
		glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
		glUniform1f(diffuseConstantLoc, diffuseConstant);
		glUniform1f(ambientStrengthLoc, ambientStrength);
	}

	// Converts the cursor position from screen coordinates to GL coordinates
	// and returns the result.
	glm::vec2 getCursorPosGL()
	{
		glm::vec2 screenPos(mouseOldX, mouseOldY);
		// Interpret click as at centre of pixel.
		glm::vec2 centredPos = screenPos + glm::vec2(0.5f, 0.5f);
		// Scale cursor position to [0, 1] range.
		glm::vec2 scaledToZeroOne = centredPos / glm::vec2(screenWidth, screenHeight);

		glm::vec2 flippedY = glm::vec2(scaledToZeroOne.x, 1.0f - scaledToZeroOne.y);

		// Go from [0, 1] range to [-1, 1] range.
		return 2.f * flippedY - glm::vec2(1.f, 1.f);
	}

	glm::vec2 cursorPosScreenCoords()
	{
		return glm::vec2(mouseOldX, mouseOldY);
	}

	int indexOfPointAtCursorPos(std::vector<Vertex>& glCoordsOfPointsToSearch, float screenCoordThreshold, Camera current) {

		std::vector<glm::vec3> screenCoordVerts;
		for (const auto& v : glCoordsOfPointsToSearch) {
			screenCoordVerts.push_back(glm::vec3(glPosToScreenCoords(current.getMousePos(glm::vec4(v.position, 1.f))), 0.f));
		}

		glm::vec2 screenMouse = cursorPosScreenCoords();
		glm::vec3 cursorPosScreen(screenMouse.x + 0.5f, screenMouse.y + 0.5f, 0.f);

		for (size_t i = 0; i < screenCoordVerts.size(); i++) {
			// Return i if length of difference vector within threshold.
			glm::vec3 diff = screenCoordVerts[i] - cursorPosScreen;
			if (glm::length(diff) < screenCoordThreshold) {
				return i;
			}
		}
		return -1; // No point within threshold found.
	}

	int onaxis_indexOfPointAtCursorPos(std::vector<Vertex>& glCoordsOfPointsToSearch, float screenCoordThreshold, Camera current) {

		std::vector<glm::vec3> screenCoordVerts;
		for (const auto& v : glCoordsOfPointsToSearch) {
			screenCoordVerts.push_back(glm::vec3(glPosToScreenCoords(current.getMousePos(glm::vec4(v.position, 1.f))), 0.f));
		}

		glm::vec2 screenMouse = cursorPosScreenCoords();
		glm::vec3 cursorPosScreen(screenMouse.x + 0.5f, screenMouse.y + 0.5f, 0.f);

		for (size_t i = 0; i < screenCoordVerts.size(); i++) {
			// Return i if length of difference vector within threshold.
			glm::vec3 diff = screenCoordVerts[i] - cursorPosScreen;
			if (glm::length(diff) < screenCoordThreshold) {
				return i;
			}
		}
		return -1; // No point within threshold found.
	}

	glm::vec3 getWorldPos() {
		glm::vec2 mouse = cursorPosScreenCoords();
		glm::mat4 M = glm::mat4(1.0);
		glm::mat4 V = camera.getView();
		glm::mat4 P = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 1000.f);
		glm::vec4 viewport = glm::vec4(0, 0, screenWidth, screenHeight);

		glm::vec3 worldPos = glm::unProject(glm::vec3(mouse, 0.f), M, P, viewport);
		return worldPos;
	}

	glm::vec3 getRayDirection() {
		glm::vec2 mouse = cursorPosScreenCoords();
		glm::mat4 M = glm::mat4(1.0);
		glm::mat4 V = camera.getView();
		glm::mat4 P = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 1000.f);
		glm::vec4 viewport = glm::vec4(0, 0, screenWidth, screenHeight);
		glm::vec3 worldPos = glm::unProject(glm::vec3(mouse, 0.f), M, P, viewport);

		glm::vec3 rayDirection = glm::normalize(glm::unProject(glm::vec3(mouse, 1.f), M, P, viewport) - worldPos);
		return rayDirection;
	}

	bool rightMouseDown;
	bool leftMouseDown;
	int currentFrame;
	int lastLeftPressedFrame;

private:
	// Uniform locations do not, ordinarily, change between frames.
	// However, we may need to update them if the shader is changed and recompiled.
	void updateUniformLocations()
	{
		lightPosLoc = glGetUniformLocation(lightingShader, "lightPos");
		ambientStrengthLoc = glGetUniformLocation(lightingShader, "ambientStrength");
		diffuseConstantLoc = glGetUniformLocation(lightingShader, "diffuseConstant");

		lightingMLoc = glGetUniformLocation(lightingShader, "M");
		lightingVLoc = glGetUniformLocation(lightingShader, "V");
		lightingPLoc = glGetUniformLocation(lightingShader, "P");

		noLightingMLoc = glGetUniformLocation(noLightingShader, "M");
		noLightingVLoc = glGetUniformLocation(noLightingShader, "V");
		noLightingPLoc = glGetUniformLocation(noLightingShader, "P");

		mLocPicker = glGetUniformLocation(pickerShader, "M");
		vLocPicker = glGetUniformLocation(pickerShader, "V");
		pLocPicker = glGetUniformLocation(pickerShader, "P");
		pickerIDLoc = glGetUniformLocation(pickerShader, "objIndex");
	}

	int screenWidth;
	int screenHeight;

	float aspect;
	double mouseOldX;
	double mouseOldY;

	// Uniform locations
	GLint lightPosLoc;
	GLint ambientStrengthLoc;
	GLint diffuseConstantLoc;

	GLint lightingMLoc;
	GLint lightingVLoc;
	GLint lightingPLoc;

	GLint noLightingMLoc;
	GLint noLightingVLoc;
	GLint noLightingPLoc;

	GLint mLocPicker;
	GLint vLocPicker;
	GLint pLocPicker;
	GLint pickerIDLoc;

	ShaderProgram &lightingShader;
	ShaderProgram &noLightingShader;
	ShaderProgram &pickerShader;
	Camera &camera;

	glm::vec2 glPosToScreenCoords(glm::vec2 glPos) {
		// Convert the [-1, 1] range to [0, 1]
		glm::vec2 scaledZeroOne = 0.5f * (glPos + glm::vec2(1.f, 1.f));

		glm::vec2 flippedY = glm::vec2(scaledZeroOne.x, 1.0f - scaledZeroOne.y);
		glm::vec2 screenPos = flippedY * glm::vec2(screenWidth, screenHeight);
		return screenPos;
	}
};

std::vector<Line> generateAxisLines()
{
	std::vector<Line> axisLines;

	Vertex xPositive{glm::vec3(50.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f)};
	Vertex xNegative{glm::vec3(-50.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f)};
	axisLines.emplace_back(std::vector<Vertex>{xNegative, xPositive});

	Vertex yPositive{glm::vec3(0.0f, 50.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f)};
	Vertex yNegative{glm::vec3(0.0f, -50.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f)};
	axisLines.emplace_back(std::vector<Vertex>{yNegative, yPositive});

	Vertex zPositive{glm::vec3(0.0f, 0.0f, 50.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f)};
	Vertex zNegative{glm::vec3(0.0f, 0.0f, -50.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f)};
	axisLines.emplace_back(std::vector<Vertex>{zNegative, zPositive});

	return axisLines;
}

void drawCurve(std::vector<Line> &lines, std::vector<Line> &points, Line controlpoints, glm::vec3 lineColor, glm::vec3 pointColor, int precision) {
	Line* lineInProgress = nullptr;
	Line* pointsInProgress = nullptr;

	lines.emplace_back(Line(controlpoints.verts));
	points.emplace_back(Line(controlpoints.verts));

	lineInProgress = &lines.back();
	lineInProgress->setColor(lineColor);
	lineInProgress->BSpline(precision, lineColor);
	lineInProgress->updateGPU();
	lineInProgress = nullptr;

	pointsInProgress = &points.back();
	pointsInProgress->setColor(glm::vec3(pointColor));
	pointsInProgress->updateGPU();
	pointsInProgress = nullptr;
}

void updateMesh(Mesh& mesh, Line bound1, Line bound2, Line profile1, Line profile2, Line crosssection, int precision, glm::vec3 color) {
	Mesh newmesh;
	Mesh tempmesh;

	if (profile1.verts.size() != 0 && profile2.verts.size() != 0) {
		newmesh.ctrlpts1 = bound1.verts;
		newmesh.ctrlpts2 = bound2.verts;
		newmesh.cam = mesh.cam;
		newmesh.sweep = crosssection.verts;
		newmesh.create(precision);
		tempmesh = newmesh.gettempmesh();

		glm::vec3 axis = newmesh.getAxis();
		// fix angle
		// first isolate to direction of up
		glm::vec3 testup = axis * mesh.cam.getUp();
		if ((testup.x + testup.y + testup.z) < 0) {
			axis = axis * (glm::vec3(-1.f));
		}
		float ptheta = glm::orientedAngle(newmesh.cam.getUp(), axis, -newmesh.cam.getPos());

		mesh.pinch1 = profile1.verts;
		tempmesh.pinch1 = profile1.verts;
		mesh.pinch2 = profile2.verts;
		tempmesh.pinch2 = profile2.verts;
		tempmesh.create(precision);
		tempmesh.updateGPU();

		mesh.verts.clear();
		for (auto j = tempmesh.verts.begin(); j < tempmesh.verts.end(); j++) {
			(*j).position = glm::translate(glm::mat4(1.f), newmesh.getCenter()) * glm::rotate(glm::mat4(1.f), ptheta, -newmesh.cam.getPos()) * glm::vec4((*j).position, 1.f);
			(*j).normal = glm::rotate(glm::mat4(1.f), ptheta, -newmesh.cam.getPos()) * glm::vec4((*j).normal, 0.f);
			mesh.verts.push_back((*j));
		}
		mesh.ctrlpts1 = bound1.verts;
		mesh.ctrlpts2 = bound2.verts;
		mesh.setColor(color);
		mesh.updateGPU();
	}

	else {
		mesh.ctrlpts1 = bound1.verts;
		mesh.ctrlpts2 = bound2.verts;
		mesh.sweep = crosssection.verts;
		mesh.create(precision);
		mesh.updateGPU();
	}
}

// return true if export was successful, false otherwise
bool exportToObj(std::string filename, std::vector<Mesh> &meshes)
{
	try
	{
		std::string verticesString;
		std::string normalsString;
		std::vector<std::string> faceGroups;

		int offset = 1;
		for (int i = 0; i < meshes.size(); i++)
		{
			Mesh &mesh = meshes[i];

			for (Vertex &vert : mesh.verts)
			{
				verticesString += "v " + std::to_string(vert.position.x) + " " + std::to_string(vert.position.y) + " " + std::to_string(vert.position.z) + "\n";
				normalsString += "vn " + std::to_string(vert.normal.x) + " " + std::to_string(vert.normal.y) + " " + std::to_string(vert.normal.z) + "\n";
			}

			std::string groupString = "g object " + std::to_string(i) + "\n";
			for (int i = 2; i < mesh.indices.size(); i += 3)
			{
				// groupString += "f " + std::to_string(mesh.indices[i - 2] + offset) + " " + std::to_string(mesh.indices[i - 1] + offset) + " " + std::to_string(mesh.indices[i] + offset) + "\n";
				groupString += "f " + std::to_string(mesh.indices[i - 2] + offset) + "//" + std::to_string(mesh.indices[i - 2] + offset) + " " + std::to_string(mesh.indices[i - 1] + offset) + "//" + std::to_string(mesh.indices[i - 1] + offset) + " " + std::to_string(mesh.indices[i] + offset) + "//" + std::to_string(mesh.indices[i] + offset) + "\n";
			}
			faceGroups.push_back(groupString);

			offset += mesh.verts.size();
		}

		std::ofstream outfile(filename);

		outfile << verticesString << std::endl;
		outfile << normalsString << std::endl;
		for (std::string &groupString : faceGroups)
		{
			outfile << groupString << std::endl;
		}

		outfile.close();
		return true;
	}
	catch (std::exception &e)
	{
		return false;
	}
}

int findSelectedObjectIndex(
	Framebuffer &pickerFB,
	Texture &pickerTex,
	ShaderProgram &pickerShader,
	std::shared_ptr<Callbacks3D> cb,
	Window &window,
	std::vector<Mesh> &meshes)
{
	int pickerClearValue[4] = {0, 0, 0, 0};

	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST);

	pickerFB.bind();

	glDisable(GL_DITHER);
	glClearBufferiv(GL_COLOR, 0, pickerClearValue);
	glClear(GL_DEPTH_BUFFER_BIT);

	const glm::ivec2 fbSize = window.getFramebufferSize();
	const glm::ivec2 winSize = window.getSize();

	const glm::ivec2 pickPosFlipped = glm::ivec2(cb->cursorPosScreenCoords()) * (fbSize / winSize);
	const glm::ivec2 pickPos(pickPosFlipped.x, fbSize.y - pickPosFlipped.y);

	glEnable(GL_SCISSOR_TEST);
	glScissor(pickPos.x, pickPos.y, 1, 1);

	pickerShader.use();
	cb->viewPipelinePicker();
	for (int i = 0; i < meshes.size(); i++)
	{
		cb->updateIDUniform(i + 1);
		meshes[i].draw();
	}

	pickerTex.bind();
	GLint pickTexCPU[1];
	glReadPixels(pickPos.x, pickPos.y, 1, 1, GL_RED_INTEGER, GL_INT, pickTexCPU);

	// Binds the "0" default framebuffer, which we use for our visual result.
	pickerFB.unbind();

	// Reset changed settings to default for the main visual render.
	glEnable(GL_DITHER);
	glDisable(GL_SCISSOR_TEST);

	return pickTexCPU[0] - 1;
}

int main()
{
	Log::debug("Starting main");

	// WINDOW
	glfwInit();
	Window window(1280, 960, "CPSC 589 Project"); // could set callbacks at construction if desired

	GLDebug::enable();

	// SHADERS
	ShaderProgram lightingShader("shaders/lighting3D.vert", "shaders/lighting3D.frag");
	ShaderProgram noLightingShader("shaders/nolighting3D.vert", "shaders/nolighting3D.frag");
	ShaderProgram pickerShader("shaders/nolighting3D.vert", "shaders/picker.frag");

	Camera cam(glm::radians(0.f), glm::radians(0.f), 3.0);
	cam.fix();
	auto cb = std::make_shared<Callbacks3D>(lightingShader, noLightingShader, pickerShader, cam, window.getWidth(), window.getHeight());

	// CALLBACKS
	window.setCallbacks(cb);

	window.setupImGui(); // Make sure this call comes AFTER GLFW callbacks set.

	// OBJECT SELECTION SETUP
	const glm::ivec2 fbSize = window.getFramebufferSize();

	Texture pickerTex(0, GL_R32I, fbSize.x, fbSize.y, GL_RED_INTEGER, GL_INT, GL_NEAREST);

	Renderbuffer pickerRB;
	pickerRB.setStorage(GL_DEPTH_COMPONENT24, fbSize.x, fbSize.y);

	Framebuffer pickerFB;
	pickerFB.addTextureAttachment(GL_COLOR_ATTACHMENT0, pickerTex);
	pickerFB.addRenderbufferAttachment(GL_DEPTH_ATTACHMENT, pickerRB);

	auto fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fbStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		Log::error("Error creating framebuffer : {}", fbStatus);
		throw std::runtime_error("Framebuffer creation error!");
	}

	glm::vec3 lightPos(0.f, 35.f, 35.f);
	float ambientStrength = 0.1f;
	float diffuseConstant = 0.7f;

	bool showAxes = true;
	bool simpleWireframe = false;
	bool showbounds = false;
	bool hide = false;
	// bool sweep = false;

	// Set the initial, default values of the shading uniforms.
	lightingShader.use();
	cb->updateShadingUniforms(lightPos, diffuseConstant, ambientStrength);

	std::vector<Line> axisLines = generateAxisLines();
	for (Line& line : axisLines)
	{
		line.updateGPU();
	}

	std::vector<Mesh> meshes;
	Mesh* meshInProgress = nullptr;

	Mesh tempmesh;
	float profiletheta;
	Camera oldcam(cam);

	glm::vec3 lineColor{ 0.0f, 1.0f, 0.7f };
	glm::vec3 stashedColor{ 0.f, 1.f, 0.7f };
	std::vector<Line> lines;
	Line* lineInProgress = nullptr;
	float pointEpsilon = 0.01f;

	glm::vec3 boundColor{ 1.0f, 0.7f, 0.0f };
	std::vector<Line> bounds;
	Line* boundInProgress = nullptr;

	glm::vec3 black{ 0.f, 0.f, 0.f };
	std::vector<Line> modify_points;
	std::vector<Line> static_points;

	Line* pointsInProgress = nullptr;

	int selectedPointIndex = -1;
	int selectedCurveIndex = -1;

	float pointSize = 5.0f;
	int precision = 150;

	std::vector<int> ptmodify = std::vector{ -1,-1 };

	char ObjFilename[] = "";
	std::string lastExportedFilename = "";

	int hoveredObjectIndex = -1;
	int selectedObjectIndex = -1;
	glm::vec3 meshCol;

	int chaikin_iter = 2;
	bool chaikin_change = true;

	enum ViewType
	{
		FREE_VIEW,
		DRAW_VIEW,
		OBJECT_VIEW,
		CURVE_VIEW,
		PROFILE_VIEW,
		PROFILE_DRAW,
		PROFILE_EDIT,
		CROSS_VIEW,
		CROSS_DRAW,
		CROSS_EDIT
	};
	ViewType view = FREE_VIEW;

	// RENDER LOOP
	while (!window.shouldClose())
	{
		cb->incrementFrameCount();
		glfwPollEvents();

		// Detect Hovered Objects in FREE_VIEW
		if (view == FREE_VIEW)
			hoveredObjectIndex = findSelectedObjectIndex(pickerFB, pickerTex, pickerShader, cb, window, meshes);
		else
			hoveredObjectIndex = -1;

		// switch for OBJECT_VIEW when a hovered object gets clicked
		if (hoveredObjectIndex >= 0 && cb->leftMouseDown)
		{
			selectedObjectIndex = hoveredObjectIndex;
			view = OBJECT_VIEW;
			meshCol = meshes[selectedObjectIndex].color;
			stashedColor = lineColor;
			lineColor = meshCol;
		}

		// Line Drawing Logic. Max 2 lines can be drawn at a time. Only in DRAW_VIEW
		if ((view == DRAW_VIEW || view == PROFILE_DRAW || view == CROSS_DRAW) && cb->leftMouseDown)
		{
			glm::vec4 cursorPos;
			// regular drawing mode (if user is drawing the object or user is making a cross section
			if (view == DRAW_VIEW || view == CROSS_DRAW || view == PROFILE_DRAW) {
				cursorPos = cam.getCursorPos(cb->getCursorPosGL());
			}

			if (lineInProgress)
			{
				// add points to line in progress
				Vertex lastVertex = lineInProgress->verts.back();
				glm::vec3 slope = glm::vec3(cursorPos) - lastVertex.position;
				float pointDistance = glm::length(slope);
				slope = glm::normalize(slope);

				for (float dist = pointEpsilon; dist <= pointDistance; dist += pointEpsilon)
				{
					Vertex newPoint = Vertex{ lastVertex.position + slope * dist, lineColor, glm::vec3(0.0f) };
					lineInProgress->verts.push_back(newPoint);
				}

				lineInProgress->updateGPU();

				// if line is in progress and user reaches the other boundary line, end the line in progress
				if (view == CROSS_DRAW) {
					int newindex = cb->indexOfPointAtCursorPos(static_points[abs(1 - selectedCurveIndex)].verts, pointSize, cam);
					if (newindex != -1) {
						lineInProgress->verts.push_back(static_points[abs(1-selectedCurveIndex)].verts[newindex]);
						selectedCurveIndex = -1;
						selectedPointIndex = -1;
						lineInProgress->updateGPU();
						lineInProgress->ChaikinAlg(chaikin_iter);

						Line mypoints = Line(lineInProgress->verts);
						lines.pop_back();
						lineInProgress = nullptr;

						Line newdiameter;
						newdiameter.verts.push_back(mypoints.verts[0]);
						newdiameter.verts.push_back(mypoints.verts.back());

						drawCurve(lines, modify_points, mypoints.verts, lineColor, black, 50);

						static_points.emplace_back(newdiameter.verts);
						lineInProgress = &static_points.back();
						lineInProgress->setColor(black);
						lineInProgress->updateGPU();

						static_points[0].setColor(black);
						static_points[0].updateGPU();
						static_points[1].setColor(black);
						static_points[1].updateGPU();

						pointsInProgress = nullptr;
						lineInProgress = nullptr;

						view = CROSS_EDIT;
					}
				}

			}
			// line drawing adds a new line if there are less than 2 lines
			else if ((view == DRAW_VIEW || view == PROFILE_DRAW) && lines.size() < 2)
			{
				// create a new line
				lines.emplace_back(std::vector<Vertex>{Vertex{ cursorPos, lineColor, glm::vec3(0.0f) }});
				lineInProgress = &lines.back();
				lineInProgress->updateGPU();
			}
			// exception is cross section drawing where the boundary lines are shown
			else if (view == CROSS_DRAW && lines.size() < 1) {
				selectedPointIndex = -1;
				for (int i = 0; i < static_points.size(); i++) {
					if (selectedPointIndex == -1) {
						selectedPointIndex = cb->indexOfPointAtCursorPos(static_points[i].verts, pointSize, cam);
						selectedCurveIndex = i;
						
						if (selectedPointIndex != -1) {
							static_points[selectedCurveIndex].verts[selectedPointIndex].color = lineColor;
							static_points[selectedCurveIndex].updateGPU();

							// create a new line
							lines.emplace_back(std::vector<Vertex>{static_points[selectedCurveIndex].verts[selectedPointIndex], Vertex{ cursorPos, lineColor, glm::vec3(0.0f) }});
							lineInProgress = &lines.back();
							lineInProgress->updateGPU();
						}
					}
				}
			}
		}
		//clean up lines, run chaikin if mouse is lifted
		else if (!(cb->leftMouseDown) && lineInProgress != nullptr)
		{
			if (view == CROSS_DRAW || lineInProgress->verts.size() < 4) {
				if (view == CROSS_DRAW) {
					static_points[0].setColor(black);
					static_points[0].updateGPU();
					static_points[1].setColor(black);
					static_points[1].updateGPU();
					selectedPointIndex = -1;
					selectedCurveIndex = -1;
				}
				lines.pop_back();
			}
			else {
				lineInProgress->ChaikinAlg(chaikin_iter);
				modify_points.emplace_back(Line(lineInProgress->verts));

				pointsInProgress = &modify_points.back();
				pointsInProgress->setColor(black);
				pointsInProgress->updateGPU();

				lineInProgress->BSpline(precision, lineColor);
				lineInProgress->updateGPU();
			}
			pointsInProgress = nullptr;
			lineInProgress = nullptr;
		}

		// grab initial point to drag points in progress
		if ((view == CURVE_VIEW || view == CROSS_EDIT || view == PROFILE_EDIT) && cb->leftMouseJustPressed()) {
			selectedPointIndex = -1;
			selectedCurveIndex = -1;
			if (view == CURVE_VIEW || view == PROFILE_EDIT || view == CROSS_EDIT) {
				for (int i = 0; i < modify_points.size(); i++) {
					if (selectedPointIndex == -1) {
						selectedPointIndex = cb->indexOfPointAtCursorPos(modify_points[i].verts, pointSize, cam);
						selectedCurveIndex = i;
					}
				}
			}
		}
		// drag points
		else if ((view == CURVE_VIEW || view == CROSS_EDIT || view == PROFILE_EDIT) && cb->leftMouseDown && selectedPointIndex != -1) {
			modify_points[selectedCurveIndex].verts[selectedPointIndex].position = cam.getCursorPos(cb->getCursorPosGL());
			modify_points[selectedCurveIndex].updateGPU();
			lines[selectedCurveIndex].verts = modify_points[selectedCurveIndex].verts;
			lines[selectedCurveIndex].BSpline(precision, lines[selectedCurveIndex].col);
			lines[selectedCurveIndex].updateGPU();
			pointsInProgress = nullptr;
			lineInProgress = nullptr;
		}


		// Start ImGui Frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		bool change = false; // Whether any ImGui variable's changed.

		// free view (has object selection, go to draw mode, or export to obj)
		if (view == FREE_VIEW)
		{
			ImGui::Begin("Free View");
			if (ImGui::Button("Draw Mode"))
			{
				view = DRAW_VIEW;
				change = true;
			}

			ImGui::Text("");
			ImGui::Text("Export to .obj");
			ImGui::InputText("Filename", ObjFilename, size_t(32));
			if (sizeof(ObjFilename) > 0 && ImGui::Button("Save"))
			{
				std::string filename = ObjFilename;
				if (filename.find(".obj") == std::string::npos)
					filename += ".obj";

				bool isSuccessful = exportToObj(filename, meshes);
				if (isSuccessful)
				{
					ImGui::OpenPopup("ExportObjSuccessPopup");
					lastExportedFilename = RUNTIME_OUTPUT_DIRECTORY + std::string("/") + filename;
					memset(ObjFilename, 0, sizeof ObjFilename);
				}
				else
				{
					ImGui::OpenPopup("ExportObjErrorPopup");
					lastExportedFilename = "";
				}
			}
		}
		// draw view (can be done in any of the 3 angles)
		else if (view == DRAW_VIEW)
		{
			ImGui::Begin("Draw Mode");
			if (ImGui::Button("Free View"))
			{
				view = FREE_VIEW;
				change = true;
				lines.clear();
			}

			ImGui::Text("");
			if (ImGui::Button("View XY Plane") && lines.size() == 0)
			{
				cam.phi = 0.f;
				cam.theta = 0.f;
			}
			if (ImGui::Button("View XZ Plane") && lines.size() == 0)
			{
				cam.phi = 0.f;
				cam.theta = M_PI_2 - 0.0001f;
			}
			if (ImGui::Button("View ZY Plane") && lines.size() == 0)
			{
				cam.phi = M_PI_2;
				cam.theta = 0.f;
			}
			ImGui::Text("");
			std::string linesDrawn = "Lines Drawn: " + std::to_string(lines.size()) + "/" + std::to_string(2);

			if (lines.size() == 2)
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
			ImGui::Text(linesDrawn.c_str());
			if (lines.size() == 2)
				ImGui::PopStyleColor();
			ImGui::ColorEdit3("New Object Color", (float*)&lineColor);
			// if there are lines on the screen you can edit the curves
			if (lines.size() > 0) {
				if (ImGui::Button("Edit Curve(s)")) {
					view = CURVE_VIEW;

					static_points.clear();

					for (auto i = modify_points.begin(); i < modify_points.end(); i++) {
						static_points.emplace_back(Line((*i).verts));
					}

				}
				// clear lines and points
				if (ImGui::Button("Clear Lines"))
				{
					static_points.clear();
					modify_points.clear();
					lines.clear();
				}
			}

			// if 2 lines, user can create
			if (lines.size() == 2)
			{
				if (ImGui::Button("Create Rotational Blending Surface"))
				{
					meshes.emplace_back();
					meshInProgress = &meshes.back();
					meshInProgress->ctrlpts1 = modify_points.back().verts;
					lines.pop_back();
					modify_points.pop_back();
					meshInProgress->ctrlpts2 = modify_points.back().verts;
					lines.pop_back();
					modify_points.pop_back();

					// sets default 'sweep'/'crosssection'
					meshInProgress->sweep = cam.getcircle(precision);
					meshInProgress->cam = cam;
					meshInProgress->create(precision);
					meshInProgress->setColor(lineColor);
					meshInProgress->updateGPU();
					meshInProgress = nullptr;
				}
			}
		}
		// if in object view
		else if (view == OBJECT_VIEW)
		{
			std::string frameTitle = "Object View - Object " + std::to_string(selectedObjectIndex);
			ImGui::Begin(frameTitle.c_str());

			ImGui::ColorEdit3("Object Color", glm::value_ptr(meshCol));

			// user can apply a color to the object
			if (ImGui::Button("Apply Color"))
			{
				meshes[selectedObjectIndex].setColor(meshCol);
			}
			// can choose to modify the object
			if (ImGui::Button("Modify Object Curves")) {
				view = CURVE_VIEW;
				// goes back to camera that the object was created in 
				cam = meshes[selectedObjectIndex].cam;

				lines.clear();
				modify_points.clear();

				drawCurve(lines, modify_points, Line(meshes[selectedObjectIndex].ctrlpts1.verts), meshes[selectedObjectIndex].color, black, precision);
				drawCurve(lines, modify_points, Line(meshes[selectedObjectIndex].ctrlpts2.verts), meshes[selectedObjectIndex].color, black, precision);
			}
			// modify the profile curves of the object
			if (ImGui::Button("Modify Object Profile")) {
				view = PROFILE_VIEW;
			
				cam = meshes[selectedObjectIndex].cam;

				tempmesh = meshes[selectedObjectIndex].gettempmesh();
				tempmesh.create(precision);
				updateMesh(tempmesh, tempmesh.ctrlpts1.verts, tempmesh.ctrlpts2.verts, meshes[selectedObjectIndex].pinch1.verts, meshes[selectedObjectIndex].pinch2.verts, meshes[selectedObjectIndex].sweep.verts, precision, tempmesh.color);
				tempmesh.crosssection = meshes[selectedObjectIndex].crosssection.verts;
				tempmesh.updateGPU();


				if (fabs(cam.phi - 0.f) < 0.1f && fabs(cam.theta - 0.f) < 0.1f) {
					oldcam = cam;
					cam.phi = M_PI_2;
					cam.theta = 0.f;
				}

				else if (fabs(cam.phi - M_PI_2) < 0.1f && fabs(cam.theta - 0.f) < 0.1f) {
					oldcam = cam;
					cam.phi = 0.f;
					cam.theta = 0.f;
				}

				else if (fabs(cam.phi - 0.f) < 0.1f && fabs(cam.theta - (M_PI_2 - 0.0001f)) < 0.1f){
					oldcam = cam;
					cam.phi = M_PI_2;
					cam.theta = 0.f;
				}

				cam.fix();
			}
			// modify cross section
			if (ImGui::Button("Modify Object Cross-Section")) {
				// gets 2 curves, makes them 'static points' that are not changeable
				view = CROSS_VIEW;
				cam = meshes[selectedObjectIndex].cam;
			}

			ImGui::Text("");
			// delete mesh
			if (ImGui::Button("Delete"))
			{
				meshes.erase(meshes.begin() + selectedObjectIndex);
				view = FREE_VIEW;
				change = true;
				selectedObjectIndex = -1;
			}
			// cancel
			if (ImGui::Button("Return to Free View"))
			{
				view = FREE_VIEW;
				change = true;
				// reset linecolor
				lineColor = stashedColor;
				selectedObjectIndex = -1;
			}
		}
		// if in Curve View
		else if (view == CURVE_VIEW) {
			if (selectedObjectIndex == -1) {
				std::string frameTitle = "Curve Modification - Object " + std::to_string(int(meshes.size()) + int(floor((lines.size() - 1) / 2)));
				ImGui::Begin(frameTitle.c_str());

				if (modify_points[0].verts.size() > 4) {
					if (ImGui::Button("Decrease Control Points")) {
						for (int i = 0; i < modify_points.size(); i++) {
							modify_points[i].ChaikinAlg(1);
							modify_points[i].updateGPU();

							lines[i].verts = modify_points[i].verts;
							lines[i].BSpline(precision, lines[i].col);
							lines[i].updateGPU();
						}
					}
				}

				if (ImGui::Button("Increase Control Points")) {
					for (int i = 0; i < modify_points.size(); i++) {
						modify_points[i].RegChaikinAlg(1);
						modify_points[i].updateGPU();

						lines[i].verts = modify_points[i].verts;
						lines[i].BSpline(precision, lines[i].col);
						lines[i].updateGPU();
					}
				}

				if (ImGui::Button("Accept Changes"))
				{
					view = DRAW_VIEW;
					change = true;
				}
				// brings back points from static points, resets 
				if (ImGui::Button("Cancel Changes")) {
					lines.clear();
					modify_points.clear();

					for (auto i = static_points.begin(); i < static_points.end(); i++) {
						modify_points.emplace_back(Line((*i).verts));
						pointsInProgress = &modify_points.back();
						pointsInProgress->updateGPU();

						lines.emplace_back(Line(pointsInProgress->verts));
						lineInProgress = &lines.back();
						lineInProgress->setColor(lineColor);
						lineInProgress->BSpline(precision, lineColor);
						lineInProgress->updateGPU();

						pointsInProgress = nullptr;
						lineInProgress = nullptr;
					}

					view = DRAW_VIEW;
				}
			}
			// accept changes pushes curves to object, updates GPU
			else {
				std::string frameTitle = "Curve Modification - Object " + std::to_string(selectedObjectIndex);
				ImGui::Begin(frameTitle.c_str());

				if (ImGui::Button("Accept Changes"))
				{
					updateMesh(meshes[selectedObjectIndex], Line(modify_points[0].verts), Line(modify_points[1].verts), Line(meshes[selectedObjectIndex].pinch1.verts), Line(meshes[selectedObjectIndex].pinch2.verts), Line(meshes[selectedObjectIndex].sweep.verts), precision, meshes[selectedObjectIndex].color);
					modify_points.clear();
					lines.clear();
					view = OBJECT_VIEW;
				}
				if (ImGui::Button("Cancel"))
				{
					modify_points.clear();
					lines.clear();
					view = OBJECT_VIEW;
				}
			}
		}

		else if (view == PROFILE_VIEW || view == PROFILE_DRAW || view == PROFILE_EDIT) {
			std::string frameTitle = "Profile Modification - Object " + std::to_string(selectedObjectIndex);
			ImGui::Begin(frameTitle.c_str());

			ImGui::Text("");

			if (view == PROFILE_VIEW) {
				if (ImGui::Button("Draw New Object Profile")) {
					view = PROFILE_DRAW;
				}
				if (ImGui::Button("Edit Existing Object Profile")) {
					view = PROFILE_EDIT;
					if (meshes[selectedObjectIndex].pinch1.verts.size() > 0 && meshes[selectedObjectIndex].pinch2.verts.size() > 0) {
						lines.clear();
						modify_points.clear();
						
						drawCurve(lines, modify_points, Line(meshes[selectedObjectIndex].pinch1.verts), meshes[selectedObjectIndex].color, black, precision);
						drawCurve(lines, modify_points, Line(meshes[selectedObjectIndex].pinch2.verts), meshes[selectedObjectIndex].color, black, precision);
					}
					else {
						lines.clear();
						modify_points.clear();

						std::vector<Line> pinches = tempmesh.getPinches(precision);

						drawCurve(lines, modify_points, Line(pinches[0].verts), meshes[selectedObjectIndex].color, black, precision);
						drawCurve(lines, modify_points, Line(pinches[1].verts), meshes[selectedObjectIndex].color, black, precision);

						pinches.clear();
					}
				}
				if (ImGui::Button("Exit to Object View")) {
					cam = oldcam;
					tempmesh.verts.clear();
					tempmesh.updateGPU();
					modify_points.clear();
					lines.clear();
					view = OBJECT_VIEW;
				}
			}

			else if (view == PROFILE_DRAW || view == PROFILE_EDIT) {
				ImGui::Text("");
				std::string linesDrawn = "Lines Drawn: " + std::to_string(lines.size()) + "/" + std::to_string(2);
				if (view == PROFILE_DRAW) {
					if (lines.size() == 2)
						ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
					ImGui::Text(linesDrawn.c_str());
					if (lines.size() == 2)
						ImGui::PopStyleColor();
				}

				if (view == PROFILE_EDIT) {
					if (modify_points[0].verts.size() > 4) {
						if (ImGui::Button("Decrease Control Points")) {
							for (int i = 0; i < modify_points.size(); i++) {
								modify_points[i].ChaikinAlg(1);
								modify_points[i].updateGPU();

								lines[i].verts = modify_points[i].verts;
								lines[i].BSpline(precision, lines[i].col);
								lines[i].updateGPU();
							}
						}
					}

					if (ImGui::Button("Increase Control Points")) {
						for (int i = 0; i < modify_points.size(); i++) {
							modify_points[i].RegChaikinAlg(1);
							modify_points[i].updateGPU();

							lines[i].verts = modify_points[i].verts;
							lines[i].BSpline(precision, lines[i].col);
							lines[i].updateGPU();
						}
					}
				}

				if (lines.size() == 2 && meshes.size() != 0) {
					if (ImGui::Button("Accept Changes")) {
						updateMesh(meshes[selectedObjectIndex], Line(meshes[selectedObjectIndex].ctrlpts1.verts), Line(meshes[selectedObjectIndex].ctrlpts2.verts), Line(modify_points[0].verts), Line(modify_points[1].verts), Line(meshes[selectedObjectIndex].sweep.verts), precision, meshes[selectedObjectIndex].color);
						
						tempmesh.pinch1 = modify_points[0].verts;
						tempmesh.pinch2 = modify_points[1].verts;
						tempmesh.create(precision);
						tempmesh.updateGPU();

						modify_points.clear();
						lines.clear();

						view = PROFILE_VIEW;
					}
				}
				if (ImGui::Button("Cancel")) {
					modify_points.clear();
					lines.clear();
					view = PROFILE_VIEW;
				}
			}

		}
		else if (view == CROSS_VIEW) {
			std::string frameTitle = "Cross-Section Modification - Object " + std::to_string(selectedObjectIndex);
			ImGui::Begin(frameTitle.c_str());

			if (ImGui::Button("Draw New Object Cross-Section")) {

				lines.clear();
				modify_points.clear();
				static_points.clear();

				static_points.emplace_back(Line(meshes[selectedObjectIndex].ctrlpts1.verts));
				pointsInProgress = &static_points.back();
				pointsInProgress->setColor(black);
				pointsInProgress->BSpline(20, black);
				pointsInProgress->updateGPU();
				pointsInProgress = nullptr;

				static_points.emplace_back(Line(meshes[selectedObjectIndex].ctrlpts2.verts));
				pointsInProgress = &static_points.back();
				pointsInProgress->setColor(black);
				pointsInProgress->BSpline(20, black);
				pointsInProgress->updateGPU();
				pointsInProgress = nullptr;

				stashedColor = lineColor;
				lineColor = meshes[selectedObjectIndex].color;

				view = CROSS_DRAW;
			}
			if (ImGui::Button("Edit Existing Object Cross-Section")) {

				lines.clear();
				modify_points.clear();
				static_points.clear();

				static_points.emplace_back(Line(meshes[selectedObjectIndex].ctrlpts1.verts));
				pointsInProgress = &static_points.back();
				pointsInProgress->setColor(black);
				pointsInProgress->BSpline(20, black);
				pointsInProgress->updateGPU();
				pointsInProgress = nullptr;

				static_points.emplace_back(Line(meshes[selectedObjectIndex].ctrlpts2.verts));
				pointsInProgress = &static_points.back();
				pointsInProgress->setColor(black);
				pointsInProgress->BSpline(20, black);
				pointsInProgress->updateGPU();
				pointsInProgress = nullptr;

				stashedColor = lineColor;
				lineColor = meshes[selectedObjectIndex].color;

				glm::vec3 p1 = static_points[0].verts[floor(static_points[0].verts.size() / 2)].position;
				glm::vec3 p2 = static_points[1].verts[floor(static_points[0].verts.size() / 2)].position;

				Line mypoints = meshes[selectedObjectIndex].getCrosssection(p1, p2, glm::vec3(1.f) - glm::abs(glm::normalize(cam.getPos())));
				if (mypoints.verts.size() > 25) {
					mypoints.ChaikinAlg(chaikin_iter);
				}
				
				drawCurve(lines, modify_points, Line(mypoints.verts), meshes[selectedObjectIndex].color, black, 50);

				Line newdiameter;
				newdiameter.verts.push_back(mypoints.verts[0]);
				newdiameter.verts.push_back(mypoints.verts.back());

				static_points.emplace_back(newdiameter.verts);
				lineInProgress = &static_points.back();
				lineInProgress->setColor(black);
				lineInProgress->updateGPU();

				lineInProgress = nullptr;

				view = CROSS_EDIT;
			}
			if (ImGui::Button("Cancel")) {
				static_points.clear();
				modify_points.clear();
				lines.clear();
				view = OBJECT_VIEW;
			}
		}
		else if (view == CROSS_EDIT || view == CROSS_DRAW) {
			std::string frameTitle = "Cross-Section Modification - Object " + std::to_string(selectedObjectIndex);
			ImGui::Begin(frameTitle.c_str());
			
			if (view == CROSS_EDIT) {
				if (modify_points[0].verts.size() > 4) {
					if (ImGui::Button("Decrease Control Points")) {
						for (int i = 0; i < modify_points.size(); i++) {
							modify_points[i].ChaikinAlg(1);
							modify_points[i].updateGPU();

							lines[i].verts = modify_points[i].verts;
							lines[i].BSpline(precision, lines[i].col);
							lines[i].updateGPU();
						}
					}
				}

				if (ImGui::Button("Increase Control Points")) {
					for (int i = 0; i < modify_points.size(); i++) {
						modify_points[i].RegChaikinAlg(1);
						modify_points[i].updateGPU();

						lines[i].verts = modify_points[i].verts;
						lines[i].BSpline(precision, lines[i].col);
						lines[i].updateGPU();
					}
				}
			}

			if (lines.size() == 1){
				if (ImGui::Button("Accept Changes")) {
					Line newcross;
					meshes[selectedObjectIndex].setcrosssection(modify_points.back().verts, glm::vec3(1.f) - glm::abs(glm::normalize(cam.getPos())), precision);
					
					updateMesh(meshes[selectedObjectIndex], meshes[selectedObjectIndex].ctrlpts1.verts, meshes[selectedObjectIndex].ctrlpts2.verts, meshes[selectedObjectIndex].pinch1.verts, meshes[selectedObjectIndex].pinch2.verts, meshes[selectedObjectIndex].sweep.verts, precision, meshes[selectedObjectIndex].color);

					lines.clear();
					modify_points.clear();
					static_points.clear();

					view = CROSS_VIEW;
				}
			}
			
			if (ImGui::Button("Cancel")) {
				modify_points.clear();
				view = CROSS_VIEW;
			}
		}

		if (ImGui::BeginPopupModal("ExportObjSuccessPopup"))
		{
			ImGui::Text("Successfully exported .obj file. File can be found here:");
			ImGui::Text(lastExportedFilename.c_str());
			ImGui::Text("");
			if (ImGui::Button("Close"))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
		else if (ImGui::BeginPopupModal("ExportObjErrorPopup"))
		{
			ImGui::Text("There was an error exporting the .obj file.");
			ImGui::Text("");
			if (ImGui::Button("Close"))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}

		// Framerate display, in case you need to debug performance.
		ImGui::Text("");
		change |= ImGui::Checkbox("Show Axes", &showAxes);
		ImGui::Text("Average %.1f ms/frame (%.1f fps)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
		ImGui::Render();

		if (change)
		{
			if (view == DRAW_VIEW)
			{
				cam.phi = 0.f;
				cam.theta = 0.f;
				cam.fix();
			}
			else if (view == FREE_VIEW)
			{
				cam.unFix();
			}
		}

		// RENDERING
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_FRAMEBUFFER_SRGB);
		glEnable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, (simpleWireframe ? GL_LINE : GL_FILL));
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Drawing Axis Lines (no Lighting)
		if (showAxes)
		{
			noLightingShader.use();
			cb->noLightingViewPipeline();
			for (Line& line : axisLines)
			{
				line.draw();
			}
		}

		// Drawing Meshes (with Lighting)
		if (view == DRAW_VIEW || view == FREE_VIEW || view == OBJECT_VIEW) {
			lightingShader.use();
			cb->lightingViewPipeline();
			for (int i = 0; i < meshes.size(); i++)
			{
				float a = ambientStrength;
				float d = diffuseConstant;
				if ((selectedObjectIndex >= 0 && i == selectedObjectIndex) || (hoveredObjectIndex >= 0 && i == hoveredObjectIndex))
				{
					d += 0.2f;
					a += 0.05f;
				}
				else if ((selectedObjectIndex >= 0 && i != selectedObjectIndex) || hoveredObjectIndex >= 0 && i != hoveredObjectIndex)
				{
					d -= 0.2;
					a -= 0.05f;
				}
				cb->updateShadingUniforms(lightPos, d, a);
				meshes[i].draw();
				//std::cout << meshes[i].getAxis() << std::endl;
			}
		}
		else if (view == CURVE_VIEW || view == PROFILE_EDIT || view == CROSS_EDIT || view == CROSS_DRAW) {}
		else if (view == PROFILE_VIEW || view == PROFILE_DRAW) {
			lightingShader.use();
			cb->lightingViewPipeline();
			float a = ambientStrength;
			float d = diffuseConstant;
			d += 0.2f;
			a += 0.05f;
			cb->updateShadingUniforms(lightPos, d, a);
			tempmesh.draw();
		}
		else {
			lightingShader.use();
			cb->lightingViewPipeline();
			float a = ambientStrength;
			float d = diffuseConstant;
			d += 0.2f;
			a += 0.05f;
			cb->updateShadingUniforms(lightPos, d, a);
			meshes[selectedObjectIndex].draw();
		}

		if (view == DRAW_VIEW || view == CURVE_VIEW || view == PROFILE_DRAW || view == PROFILE_EDIT || view == CROSS_DRAW || view == CROSS_EDIT) {
			// Drawing Lines (no Lighting)
			noLightingShader.use();
			cb->noLightingViewPipeline();
			for (Line& line : lines)
			{
				line.draw();
			}
		}

		// Drawing Control Points (no Lighting)
		if (view == CURVE_VIEW || view == PROFILE_EDIT || view == CROSS_DRAW || view == CROSS_EDIT) {
			noLightingShader.use();
			cb->noLightingViewPipeline();
			for (Line& ctrl : modify_points)
			{
				ctrl.drawPoints(pointSize);
			}
			if (view == CROSS_EDIT || view == CROSS_DRAW) {
				noLightingShader.use();
				cb->noLightingViewPipeline();
				for (Line& ctrl : static_points)
				{
					ctrl.draw();
					ctrl.drawPoints(pointSize);
				}
			}
		}

		// Drawing Lines (no Lighting)
		if (showbounds){
			noLightingShader.use();
			cb->noLightingViewPipeline();
			for (Line &bound : bounds)
			{
				bound.draw();
			}
		}

		glDisable(GL_FRAMEBUFFER_SRGB); // disable sRGB for things like imgui

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		window.swapBuffers();
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}
