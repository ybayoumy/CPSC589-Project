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

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

// EXAMPLE CALLBACKS
class Callbacks3D : public CallbackInterface {

public:
	// Constructor. We use values of -1 for attributes that, at the start of
	// the program, have no meaningful/"true" value.
	Callbacks3D(ShaderProgram& shader, Camera& camera, int screenWidth, int screenHeight)
		: shader(shader)
		, camera(camera)
		, currentFrame(0)
		, rightMouseDown(false)
		, leftMouseDown(false)
		, lastLeftPressedFrame(-1)
		, mouseOldX(-1.0)
		, mouseOldY(-1.0)
		, screenWidth(screenWidth)
		, screenHeight(screenHeight)
		, aspect(screenWidth/screenHeight)
	{
		updateUniformLocations();
	}

	virtual void keyCallback(int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_R && action == GLFW_PRESS) {
			shader.recompile();
			updateUniformLocations();
		}
	}

	virtual void mouseButtonCallback(int button, int action, int mods) {
		// If we click the mouse on the ImGui window, we don't want to log that
		// here. But if we RELEASE the mouse over the window, we do want to
		// know that!
		auto& io = ImGui::GetIO();
		if (io.WantCaptureMouse && action == GLFW_PRESS) return;

		if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			if (action == GLFW_PRESS) {
				rightMouseDown = true;
			}
			else if (action == GLFW_RELEASE)	rightMouseDown = false;
		}

		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			if (action == GLFW_PRESS) {
				leftMouseDown = true;
				lastLeftPressedFrame = currentFrame;
			}
			else if (action == GLFW_RELEASE)	leftMouseDown = false;
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
	virtual void windowSizeCallback(int width, int height) {
		screenWidth = width;
		screenHeight = height;
		aspect = float(width) / float(height);
	}

	virtual void cursorPosCallback(double xpos, double ypos) {
		if (rightMouseDown) {
			camera.incrementTheta(ypos - mouseOldY);
			camera.incrementPhi(xpos - mouseOldX);
		}
		mouseOldX = xpos;
		mouseOldY = ypos;
	}
	virtual void scrollCallback(double xoffset, double yoffset) {
		camera.incrementR(yoffset);
	}

	void viewPipeline() {
		glm::mat4 M = glm::mat4(1.0);
		glm::mat4 V = camera.getView();
		glm::mat4 P = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 1000.f);
		glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(M));
		glUniformMatrix4fv(vLoc, 1, GL_FALSE, glm::value_ptr(V));
		glUniformMatrix4fv(pLoc, 1, GL_FALSE, glm::value_ptr(P));
	}

	void updateShadingUniforms(
		const glm::vec3& lightPos, const glm::vec3& lightCol, float ambientStrength
	)
	{
		// Like viewPipeline(), this function assumes shader.use() was called before.
		glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
		glUniform3f(lightColLoc, lightCol.r, lightCol.g, lightCol.b);
		glUniform1f(ambientStrengthLoc, ambientStrength);
	}

	// Converts the cursor position from screen coordinates to GL coordinates
	// and returns the result.
	glm::vec2 getCursorPosGL() {
		glm::vec2 screenPos(mouseOldX, mouseOldY);
		// Interpret click as at centre of pixel.
		glm::vec2 centredPos = screenPos + glm::vec2(0.5f, 0.5f);
		// Scale cursor position to [0, 1] range.
		glm::vec2 scaledToZeroOne = centredPos / glm::vec2(screenWidth, screenHeight);

		glm::vec2 flippedY = glm::vec2(scaledToZeroOne.x, 1.0f - scaledToZeroOne.y);

		// Go from [0, 1] range to [-1, 1] range.
		return 2.f * flippedY - glm::vec2(1.f, 1.f);
	}

	int indexOfPointAtCursorPos(std::vector<Vertex>& glCoordsOfPointsToSearch, glm::vec2 cursorPos, float screenCoordThreshold, Camera current) {

		std::vector<glm::vec3> screenCoordVerts;
		for (const auto& v : glCoordsOfPointsToSearch) {
			screenCoordVerts.push_back(glm::vec3(glPosToScreenCoords(current.getMousePos(glm::vec4(v.position, 1.f))), 0.f));
		}

		glm::vec2 screenMouse = glPosToScreenCoords(cursorPos);
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

	bool rightMouseDown;
	bool leftMouseDown;
	int currentFrame;
	int lastLeftPressedFrame;

private:
	// Uniform locations do not, ordinarily, change between frames.
	// However, we may need to update them if the shader is changed and recompiled.
	void updateUniformLocations() {
		mLoc = glGetUniformLocation(shader, "M");
		vLoc = glGetUniformLocation(shader, "V");
		pLoc = glGetUniformLocation(shader, "P");
		lightPosLoc = glGetUniformLocation(shader, "lightPos");
		lightColLoc = glGetUniformLocation(shader, "lightCol");
		ambientStrengthLoc = glGetUniformLocation(shader, "ambientStrength");
	}

	int screenWidth;
	int screenHeight;

	float aspect;
	double mouseOldX;
	double mouseOldY;

	// Uniform locations
	GLint mLoc;
	GLint vLoc;
	GLint pLoc;
	GLint lightPosLoc;
	GLint lightColLoc;
	GLint ambientStrengthLoc;

	ShaderProgram& shader;
	Camera& camera;

	glm::vec2 glPosToScreenCoords(glm::vec2 glPos) {
		// Convert the [-1, 1] range to [0, 1]
		glm::vec2 scaledZeroOne = 0.5f * (glPos + glm::vec2(1.f, 1.f));

		glm::vec2 flippedY = glm::vec2(scaledZeroOne.x, 1.0f - scaledZeroOne.y);
		glm::vec2 screenPos = flippedY * glm::vec2(screenWidth, screenHeight);
		return screenPos;
	}
};

std::vector<Line> generateAxisLines() {
	std::vector<Line> axisLines;

	Vertex xPositive{ glm::vec3(50.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f) };
	Vertex xNegative{ glm::vec3(-50.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f) };
	axisLines.emplace_back(std::vector<Vertex>{ xNegative, xPositive });

	Vertex yPositive{ glm::vec3(0.0f, 50.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f) };
	Vertex yNegative{ glm::vec3(0.0f, -50.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f) };
	axisLines.emplace_back(std::vector<Vertex>{ yNegative, yPositive });

	Vertex zPositive{ glm::vec3(0.0f, 0.0f, 50.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f) };
	Vertex zNegative{ glm::vec3(0.0f, 0.0f, -50.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f) };
	axisLines.emplace_back(std::vector<Vertex>{ zNegative, zPositive });

	return axisLines;
}

// return true if export was successful, false otherwise
bool exportToObj(std::string filename, std::vector<Mesh>& meshes) {
	try {
		std::string verticesString;
		std::string normalsString;
		std::vector<std::string> faceGroups;

		int offset = 1;
		for (int i = 0; i < meshes.size(); i++) {
			Mesh& mesh = meshes[i];

			for (Vertex& vert : mesh.verts) {
				verticesString += "v " + std::to_string(vert.position.x) + " " + std::to_string(vert.position.y) + " " + std::to_string(vert.position.z) + "\n";
				normalsString += "vn " + std::to_string(vert.normal.x) + " " + std::to_string(vert.normal.y) + " " + std::to_string(vert.normal.z) + "\n";
			}

			std::string groupString = "g object " + std::to_string(i) + "\n";
			for (int i = 2; i < mesh.indices.size(); i += 3) {
				//groupString += "f " + std::to_string(mesh.indices[i - 2] + offset) + " " + std::to_string(mesh.indices[i - 1] + offset) + " " + std::to_string(mesh.indices[i] + offset) + "\n";
				groupString += "f " + std::to_string(mesh.indices[i - 2] + offset) + "//" + std::to_string(mesh.indices[i - 2] + offset) + " " + std::to_string(mesh.indices[i - 1] + offset) + "//" + std::to_string(mesh.indices[i - 1] + offset) + " " + std::to_string(mesh.indices[i] + offset) + "//" + std::to_string(mesh.indices[i] + offset) + "\n";
			}
			faceGroups.push_back(groupString);

			offset += mesh.verts.size();
		}

		std::ofstream outfile(filename);

		outfile << verticesString << std::endl;
		outfile << normalsString << std::endl;
		for (std::string& groupString : faceGroups) {
			outfile << groupString << std::endl;
		}

		outfile.close();
		return true;
	}
	catch (std::exception& e) {
		return false;
	}

}

int main() {
	Log::debug("Starting main");

	// WINDOW
	glfwInit();
	Window window(1280, 960, "CPSC 589 Project"); // could set callbacks at construction if desired

	GLDebug::enable();

	// SHADERS
	ShaderProgram lightingShader("shaders/lighting3D.vert", "shaders/lighting3D.frag");
	ShaderProgram noLightingShader("shaders/nolighting3D.vert", "shaders/nolighting3D.frag");

	Camera cam(glm::radians(0.f), glm::radians(0.f), 3.0);
	cam.fix();
	auto cb = std::make_shared<Callbacks3D>(lightingShader, cam, window.getWidth(), window.getHeight());

	// CALLBACKS
	window.setCallbacks(cb);

	window.setupImGui(); // Make sure this call comes AFTER GLFW callbacks set.

	glm::vec3 lightPos(0.f, 35.f, 35.f);
	glm::vec3 lightCol(1.f);
	float ambientStrength = 0.035f;

	bool inEditMode = false;
	bool inDrawMode = true;
	bool showAxes = true;
	bool simpleWireframe = false;
	bool showbounds = false;
	bool hide = false;
	bool cross = false;
	//bool sweep = false;

	// Set the initial, default values of the shading uniforms.
	lightingShader.use();
	cb->updateShadingUniforms(lightPos, lightCol, ambientStrength);

	std::vector<Line> axisLines = generateAxisLines();
	for (Line& line : axisLines) {
		line.updateGPU();
	}

	std::vector<Mesh> meshes;
	Mesh* meshInProgress = nullptr;

	glm::vec3 lineColor{ 0.0f, 1.0f, 0.7f };
	std::vector<Line> lines;
	std::vector<Line> points;
	std::vector<Line> axis;

	Line* pointsInProgress = nullptr;
	Line* lineInProgress = nullptr;
	float pointEpsilon = 0.01f;

	glm::vec3 boundColor{ 1.0f, 0.7f, 0.0f };
	std::vector<Line> bounds;
	Line* boundInProgress = nullptr;
	
	glm::vec3 black{ 0.f, 0.f, 0.f };
	float pointSize = 5.0f;
	int selectedPointIndex = -1; // Used for point dragging & deletion
	glm::vec2 editing = glm::vec2(-1, -1);

	int precision = 60;
	bool updatebounds = false;
	bool crossing = false;

	//std::vector<Line> pinch;
	//pinch.push_back(Line());
	//pinch.push_back(Line());

	//std::vector<Line> sweeps;

	//std::vector<glm::vec3> views;
	//std::vector<glm::vec3> ups;

	//std::vector<glm::vec3> direction;
	//bool XZ = false;

	//int meshchoice = 0;

	char ObjFilename[] = "";
	std::string lastExportedFilename = "";

	// RENDER LOOP
	while (!window.shouldClose()) {

		cb->incrementFrameCount();
		glfwPollEvents();

		// Line Drawing Logic. Max 2 lines can be drawn at a time
		if ((inDrawMode || cross) && cb->leftMouseDown) {
			float threshold = pointSize;
			glm::vec4 cursorPos = cam.getCursorPos(cb->getCursorPosGL());

			if (lineInProgress) {
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

				/*if (crossing) {
					int selected = cb->indexOfPointAtCursorPos(bounds[0].verts, cb->getCursorPosGL(), threshold, cam);
					if (selected == -1) {
						crossing = false;
						lineInProgress = nullptr;
					}
				}*/
			}
			else if (lines.size() < 2) {
				// create a new line
				lines.emplace_back(std::vector<Vertex>{ Vertex{ cursorPos, lineColor, glm::vec3(0.0f) } });
				lineInProgress = &lines.back();
				lineInProgress->updateGPU();
			}
		}
		else if ((!inDrawMode && !cross) || !cb->leftMouseDown) {
			if (lineInProgress != nullptr) {
				crossing = false;

				points.emplace_back(Line(lineInProgress->verts));
				pointsInProgress = &points.back();
				pointsInProgress->ChaikinAlg(2, black);

				lineInProgress->verts = (pointsInProgress->BSpline(precision, lineColor));

				lineInProgress->updateGPU();
				pointsInProgress->updateGPU();

				lineInProgress = nullptr;
				pointsInProgress = nullptr;
			}
		}
		

		// POINT SELECTION (IN EDIT MODE ONLY)
		if (inEditMode && cb->leftMouseJustPressed()) {
			float threshold = pointSize;
			for (int i = 0; i < points.size(); i++) {
				selectedPointIndex = cb->indexOfPointAtCursorPos(points[i].verts, cb->getCursorPosGL(), threshold, cam);
				if (selectedPointIndex != -1) {
					editing = glm::vec2(i, selectedPointIndex);
				}
			}
		}

		/*else if (inEditMode && cb->leftMouseJustPressed() && cross && meshes.size() > 0) {
			float threshold = pointSize;
			for (int i = 0; i < bounds.size(); i++) {
				selectedPointIndex = cb->indexOfPointAtCursorPos(bounds[i].verts, cb->getCursorPosGL(), threshold, cam);
				if (selectedPointIndex != -1) {
					crossing = true;
					lines.emplace_back(std::vector<Vertex>{ Vertex{ cam.getCursorPos(cb->getCursorPosGL()), lineColor, glm::vec3(0.0f) } });
					lineInProgress = &lines.back();
					lineInProgress->updateGPU();

					std::vector<Vertex> connect;
					connect.emplace_back(meshes[0].bound1.verts[selectedPointIndex]);
					connect.emplace_back(meshes[0].bound2.verts[selectedPointIndex]);

					bounds.clear();
					bounds.emplace_back(Line(connect));
					boundInProgress = &bounds.back();
					boundInProgress->updateGPU();
					boundInProgress = nullptr;
				}
			}
		}*/



		if (inEditMode && cb->leftMouseDown && editing.y >= 0 && (lines.size() > 0 || bounds.size() > 0)) {
			if (bounds.size() > 0) {
				int pointindex = meshes[editing.x].axis.match(cam.getCursorPos(cb->getCursorPosGL()), cam);

				points[editing.x].verts[editing.y].position = meshes[editing.x].axis.verts[pointindex].position;
				bounds[editing.y].verts = meshes[editing.x].discs[pointindex].verts;

				points[editing.x].updateGPU();
				bounds[editing.y].updateGPU();

				meshes[editing.x].sweepindex[selectedPointIndex] = pointindex;

				updatebounds = true;
			}
			else {
				// Drag selected point (for 2d editing)
				points[editing.x].verts[editing.y].position = cam.getCursorPos(cb->getCursorPosGL());
				points[editing.x].updateGPU();

				lines[editing.x].verts = points[editing.x].BSpline(precision, lineColor);
				lines[editing.x].updateGPU();
			}
		}
		else {
			editing = glm::vec2(-1, -1);
		}
		
		if (inEditMode && meshes.size() > 0 && updatebounds) {
			bounds.clear();
			for (auto i = meshes.begin(); i < meshes.end(); i++) {
				
				bounds.emplace_back(Line((*i).discs[0].verts));
				boundInProgress = &bounds.back();
				boundInProgress->updateGPU();
				boundInProgress = nullptr;

				bounds.emplace_back(Line((*i).discs.back().verts));
				boundInProgress = &bounds.back();
				boundInProgress->updateGPU();
				boundInProgress = nullptr;

				bounds.emplace_back(Line((*i).axis.verts));
				boundInProgress = &bounds.back();
				boundInProgress->updateGPU();
				boundInProgress = nullptr;

				bounds.emplace_back(Line((*i).bound1.verts));
				boundInProgress = &bounds.back();
				boundInProgress->updateGPU();
				boundInProgress = nullptr;

				bounds.emplace_back(Line((*i).bound2.verts));
				boundInProgress = &bounds.back();
				boundInProgress->updateGPU();
				boundInProgress = nullptr;

				std::vector<Vertex> endpoints;
				endpoints.push_back((*i).axis.verts[0]);
				endpoints.push_back((*i).axis.verts.back());
				points.emplace_back(Line(endpoints));
				pointsInProgress = &points.back();
				pointsInProgress->updateGPU();
				pointsInProgress = nullptr;

			}
			updatebounds = false;
		}
		//std::cout << editing.y << std::endl;

		// Three functions that must be called each new frame.
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		std::string imguiWindowTitle = "";
		if (inDrawMode) imguiWindowTitle = "Draw Mode";
		else if (inEditMode) imguiWindowTitle = "Edit Mode";
		else imguiWindowTitle = "Free View";

		ImGui::Begin(imguiWindowTitle.c_str());

		bool change = false; // Whether any ImGui variable's changed.

		//change |= ImGui::ColorEdit3("Diffuse colour", glm::value_ptr(diffuseCol));

		// The rest of our ImGui widgets.
		//change |= ImGui::DragFloat3("Light's position", glm::value_ptr(lightPos));
		//change |= ImGui::ColorEdit3("Light's colour", glm::value_ptr(lightCol));
		//change |= ImGui::SliderFloat("Ambient strength", &ambientStrength, 0.0f, 1.f);
		// change |= ImGui::Checkbox("Show Wireframe", &simpleWireframe);
		// change |= ImGui::Checkbox("Show Bounds (for Debug)", &showbounds);

		if (!inDrawMode) {
			if (ImGui::Button("Draw Mode")) {
				inDrawMode = true;
				inEditMode = false;
				change = true;
			}
			//ImGui::Checkbox("Hide", &hide);
		}
		else {
			if (ImGui::Button("Free View")) {
				inDrawMode = false;
				inEditMode = false;
				change = true;
				lines.clear();
				points.clear();
			}
			if (ImGui::Button("Edit Mode")) {
				inDrawMode = false;
				inEditMode = true;
			}
		}

		if (inEditMode) {
			if (ImGui::Button("Free View")) {
				inDrawMode = false;
				inEditMode = false;
				change = true;
				lines.clear();
				points.clear();
			}
		}

		if (inDrawMode || inEditMode) {
			ImGui::Text("");
			if (ImGui::Button("View XY Plane") && lines.size() == 0) {
				cam.phi = 0.f;
				cam.theta = 0.f;
			}
			if (ImGui::Button("View XZ Plane") && lines.size() == 0) {
				cam.phi = 0.f;
				cam.theta = M_PI_2 - 0.0001f;
			}
			if (ImGui::Button("View ZY Plane") && lines.size() == 0) {
				cam.phi = M_PI_2;
				cam.theta = 0.f;
			}
			ImGui::Text("");
			std::string linesDrawn = "Lines Drawn: " + std::to_string(lines.size()) + "/" + std::to_string(2);

			if (lines.size() == 2) ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
			ImGui::Text(linesDrawn.c_str());
			if (lines.size() == 2) ImGui::PopStyleColor();
			if (ImGui::Button("Clear Lines")) {
				lines.clear();
			}

			ImGui::ColorEdit3("New Object Color", (float*)&lineColor);
		}

		if (inEditMode) {
			ImGui::Checkbox("Edit Cross Section Shape", &hide);
			ImGui::Checkbox("Add New Cross Section", &cross);
		}
		else {
			hide = false;
		}

		if (meshes.size() > 0) {
			if (lines.size() == 1) {
				if (ImGui::Button("Update Sweep")) {
					meshes[0].sweep = meshes[0].cam.standardize(lines.back().verts);
					lines.pop_back();
					meshes[0].create(precision, meshes[0].mycolor);
					meshes[0].updateGPU();
				}
			}
		}

		//		bounds.emplace_back();
		//		boundInProgress = &bounds.back();
		//		for (auto i = (meshes[meshchoice - 1].sweep).verts.begin(); i < (meshes[meshchoice - 1].sweep).verts.end(); i++) {
		//			boundInProgress->verts.push_back(Vertex{ (*i).position, glm::vec3(1.f, 0.7f, 0.f), (*i).normal });
		//		}
		//		boundInProgress->updateGPU();
		//		boundInProgress = nullptr;
		//	}
		//}

		//if (lines.size() % 2 == 0 && lines.size() != 0) {
		//	if (ImGui::Button("Update Pinch") && meshchoice != 0) {

		//		meshes[meshchoice - 1].pinch1 = lines.back().verts;
		//		lines.pop_back();
		//		meshes[meshchoice - 1].pinch2 = lines.back().verts;
		//		lines.pop_back();
		//		meshes[meshchoice - 1].create(75);
		//		meshes[meshchoice - 1].updateGPU();


		//		//sweep = true;
		//	}
		//}

		if (lines.size() == 2) {
			if (ImGui::Button("Create Rotational Blending Surface")) {
				meshes.emplace_back();
				meshInProgress = &meshes.back();
				meshInProgress->bound1 = lines.back().verts;
				lines.pop_back();
				points.pop_back();
				meshInProgress->bound2 = lines.back().verts;
				lines.pop_back();
				points.pop_back();
				meshInProgress->sweep = cam.getcircle(floor(precision/2));
				meshInProgress->cam = cam;
				meshInProgress->create(precision, lineColor);
				meshInProgress->updateGPU();

				updatebounds = true;
				/*bounds.emplace_back();
				boundInProgress = &bounds.back();
				boundInProgress->verts.push_back(Vertex{meshInProgress->bound1.verts.back().position, glm::vec3(1.f, 0.7f, 0.f), glm::vec3(0.f) });
				boundInProgress->verts.push_back(Vertex{meshInProgress->bound2.verts.back().position, glm::vec3(1.f, 0.7f, 0.f), glm::vec3(0.f) });
				boundInProgress->updateGPU();
				boundInProgress = nullptr;*/

				/*bounds.emplace_back();
				boundInProgress = &bounds.back();
				for (auto i = (meshInProgress->bound2).verts.begin(); i < (meshInProgress->bound2).verts.end(); i++) {
					boundInProgress->verts.push_back(Vertex{ (*i).position, glm::vec3(1.f, 0.7f, 0.f), (*i).normal });
				}
				boundInProgress->updateGPU();
				boundInProgress = nullptr;

				bounds.emplace_back();
				boundInProgress = &bounds.back();
				for (auto i = (meshInProgress->sweep).verts.begin(); i < (meshInProgress->sweep).verts.end(); i++) {
					boundInProgress->verts.push_back(Vertex{ (*i).position, glm::vec3(1.f, 0.7f, 0.f), (*i).normal });
				}
				boundInProgress->updateGPU();
				boundInProgress = nullptr;

				meshInProgress = nullptr;
				*/
			}
		}

		if (!inDrawMode && !inEditMode) {
			ImGui::Text("");
			ImGui::Text("Export to .obj");
			ImGui::InputText("Filename", ObjFilename, size_t(32));
			if (sizeof(ObjFilename) > 0 && ImGui::Button("Save")) {
				std::string filename = ObjFilename;
				if (filename.find(".obj") == std::string::npos) filename += ".obj";

				bool isSuccessful = exportToObj(filename, meshes);
				if (isSuccessful) {
					ImGui::OpenPopup("ExportObjSuccessPopup");
					lastExportedFilename = RUNTIME_OUTPUT_DIRECTORY + std::string("/") + filename;
					memset(ObjFilename, 0, sizeof ObjFilename);
				}
				else {
					ImGui::OpenPopup("ExportObjErrorPopup");
					lastExportedFilename = "";
				}
			}
		}

		if (ImGui::BeginPopupModal("ExportObjSuccessPopup")) {
			ImGui::Text("Successfully exported .obj file. File can be found here:");
			ImGui::Text(lastExportedFilename.c_str());
			ImGui::Text("");
			if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
		else if (ImGui::BeginPopupModal("ExportObjErrorPopup")) {
			ImGui::Text("There was an error exporting the .obj file.");
			ImGui::Text("");
			if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}

		// Framerate display, in case you need to debug performance.
		ImGui::Text("");
		change |= ImGui::Checkbox("Show Axes", &showAxes);
		ImGui::Text("Average %.1f ms/frame (%.1f fps)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
		ImGui::Render();

		//if (sweep) {
		//	bounds.emplace_back();
		//	boundInProgress = &bounds.back();

		//	if (meshchoice == 0) {
		//		if (sweep) {
		//			sweeps[0].standardizesweep(ups[0], cam.getPos(), glm::vec3(1.f, 0.7f, 0.f));
		//		}
		//		for (auto i = sweeps[0].verts.begin(); i < sweeps[0].verts.end(); i++) {
		//			boundInProgress->verts.push_back((*i));
		//		}
		//	}
		//	else {
		//		if (sweep) {
		//			sweeps[2 * (meshchoice - 1)].standardizesweep(ups[2 * (meshchoice - 1)], cam.getPos(), glm::vec3(1.f, 0.7f, 0.f));
		//		}
		//		for (auto i = sweeps[2 * (meshchoice - 1)].verts.begin(); i < sweeps[2 * (meshchoice - 1)].verts.end(); i++) {
		//			boundInProgress->verts.push_back((*i));
		//		}
		//	}

		//	boundInProgress->updateGPU();
		//	boundInProgress = nullptr;
		//	sweep = false;
		//}

		if (change) {
			if (inDrawMode) {
				cam.phi = 0.f;
				cam.theta = 0.f;
				cam.fix();
			}
			else {
				cam.unFix();
			}
		}

		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_FRAMEBUFFER_SRGB);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, (simpleWireframe ? GL_LINE : GL_FILL));

		// Drawing Axis Lines (no Lighting)
		if (showAxes) {
			noLightingShader.use();
			cb->viewPipeline();
			for (Line& line : axisLines) {
				line.draw(noLightingShader);
			}
		}

		// Drawing Meshes (with Lighting)
		lightingShader.use();
		if (change)
		{
			cb->updateShadingUniforms(lightPos, lightCol, ambientStrength);
		}
		cb->viewPipeline();
		for (Mesh& mesh : meshes) {
			if (!hide) {
				mesh.draw(lightingShader);
			}
		}

		// Drawing Lines (no Lighting)
		noLightingShader.use();
		cb->viewPipeline();

		if (inDrawMode || inEditMode) {
			for (Line& line : lines) {
				line.draw(noLightingShader);
			}
		}

		if (inEditMode) {
			for (Line& set : points) {
				set.drawPoints(pointSize, noLightingShader);
			}
		}

		// Drawing Lines (no Lighting)
		if (inEditMode && meshes.size() > 0) {
			for (Line& bound : bounds) {
				bound.draw(noLightingShader);
				bound.drawPoints(pointSize, noLightingShader);
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
