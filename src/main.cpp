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

std::vector<glm::vec3> listOfMouseLocations;
std::vector<glm::vec3> listOfChaikinPoints;

std::vector<glm::vec3> reverseChaikinAlgorithm(std::vector<glm::vec3> mousePoints) {
	double xj, yj;
	std::vector<glm::vec3> Chaikin;
	for (int i = 0; i < mousePoints.size(); i += 3) {
		xj = 0;
		yj = 0;
		if ((0 <= (i - 1)) && ((i - 1) <= mousePoints.size())) {
			xj -= 0.25 * mousePoints[i - 1].x;
			yj -= 0.25 * mousePoints[i - 1].y;
		}
		xj += 0.75 * mousePoints[i].x;
		yj += 0.75 * mousePoints[i].y;
		if ((0 <= (i + 1)) && ((i + 1) <= mousePoints.size())) {
			xj += 0.75 * mousePoints[i + 1].x;
			yj += 0.75 * mousePoints[i + 1].y;
		}
		if ((0 <= (i + 2)) && ((i + 2) <= mousePoints.size())) {
			xj -= 0.25 * mousePoints[i + 2].x;
			yj -= 0.25 * mousePoints[i + 2].y;
		}
		Chaikin.push_back(glm::vec3(xj, yj, 0));
	}
	return Chaikin;
}

Mesh unitSphere(int granularity, glm::vec3 col) {
	float angleStep = M_PI / granularity;

	Mesh res;

	// calculating points
	for (int j = 0; j < 2 * granularity + 1; j++) {
		float phi = angleStep * j;
		for (int i = 0; i < granularity + 1; i++) {
			float theta = angleStep * i;
			glm::vec3 point(glm::sin(theta) * glm::sin(phi), glm::cos(theta), glm::sin(theta) * glm::cos(phi));
			res.verts.push_back(Vertex{ point, col, point });
		}
	}

	// creating faces using vertex indices
	for (int i = 1; i < granularity + 1; i++) {
		for (int j = 0; j < 2 * granularity + 1; j++) {

			if (j != 0) {
				res.indices.push_back((granularity + 1) * j + i);
				res.indices.push_back((granularity + 1) * (j - 1) + i);
				res.indices.push_back((granularity + 1) * j + i - 1);
			}

			if (j != 2 * granularity) {
				res.indices.push_back((granularity + 1) * j + i);
				res.indices.push_back((granularity + 1) * j + i - 1);
				res.indices.push_back((granularity + 1) * (j + 1) + i - 1);
			}
		}
	}

	return res;
}

// EXAMPLE CALLBACKS
class Callbacks3D : public CallbackInterface {

public:
	// Constructor. We use values of -1 for attributes that, at the start of
	// the program, have no meaningful/"true" value.
	Callbacks3D(ShaderProgram& shader, Camera& camera, int screenWidth, int screenHeight)
		: shader(shader)
		, camera(camera)
		, aspect(1.0f)
		, rightMouseDown(false)
		, leftMouseDown(false)
		, mouseOldX(-1.0)
		, mouseOldY(-1.0)
		, screenWidth(screenWidth)
		, screenHeight(screenHeight)
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
			if (action == GLFW_PRESS)			rightMouseDown = true;
			else if (action == GLFW_RELEASE)	rightMouseDown = false;
		}

		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			if (action == GLFW_PRESS)			leftMouseDown = true;
			else if (action == GLFW_RELEASE)	leftMouseDown = false;
		}
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
		if (leftMouseDown) {
			listOfMouseLocations.push_back(glm::vec3(xpos, ypos, 0));
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

	bool rightMouseDown;
	bool leftMouseDown;

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
};

std::vector<glm::vec3> makecircle(glm::vec3 up, glm::vec2 cam) {
	int tinc = 24;
	std::vector<glm::vec3> unitcircle;
	for (int i = 0; i < tinc; i++) {
		float angle = i * 2 * M_PI / tinc;
		glm::mat4 R = glm::rotate(glm::mat4(1.f), cam.x + float(M_PI/2), up);
		glm::vec3 point = R * glm::vec4(cos(angle), sin(angle), 0, 1.f);
		unitcircle.push_back(point);
	}
	return unitcircle;
}

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

void exportToObj(std::string filename, std::vector<Mesh>& meshes) {
	std::string verticesString;
	std::string normalsString;
	std::vector<std::string> faceGroups;

	int offset = 0;
	for (int i = 0; i < meshes.size(); i++) {
		for (Vertex& vert : meshes[i].verts) {
			verticesString += "v " + std::to_string(vert.position.x) + " " + std::to_string(vert.position.y) + " " + std::to_string(vert.position.z) + "\n";
			normalsString += "vn " + std::to_string(vert.normal.x) + " " + std::to_string(vert.normal.y) + " " + std::to_string(vert.normal.z) + "\n";
		}

		std::string groupString = "g object " + std::to_string(i) + "\n";
		for (int i = 2; i < meshes[i].indices.size(); i += 3) {
			groupString += "f " + std::to_string(meshes[i].indices[i - 2] + offset) + "//" + std::to_string(meshes[i].indices[i - 2] + offset) + " " + std::to_string(meshes[i].indices[i - 1] + offset) + "//" + std::to_string(meshes[i].indices[i - 1] + offset) + " " + std::to_string(meshes[i].indices[i] + offset) + "//" + std::to_string(meshes[i].indices[i] + offset) + "\n";
		}
		offset += meshes[i].verts.size();
	}

	std::ofstream outfile(filename);

	outfile << verticesString << std::endl;
	outfile << normalsString << std::endl;
	for (std::string& groupString : faceGroups) {
		outfile << groupString << std::endl;
	}

	outfile.close();
}

int main() {
	Log::debug("Starting main");

	// WINDOW
	glfwInit();
	Window window(800, 800, "CPSC 589 Project"); // could set callbacks at construction if desired

	GLDebug::enable();

	// SHADERS
	ShaderProgram lightingShader("shaders/lighting3D.vert", "shaders/lighting3D.frag");
	ShaderProgram noLightingShader("shaders/nolighting3D.vert", "shaders/nolighting3D.frag");

	Camera cam(glm::radians(0.f), glm::radians(0.f), 3.0);
	auto cb = std::make_shared<Callbacks3D>(lightingShader, cam, window.getWidth(), window.getHeight());

	// CALLBACKS
	window.setCallbacks(cb);

	window.setupImGui(); // Make sure this call comes AFTER GLFW callbacks set.

	glm::vec3 lightPos(0.f, 35.f, 35.f);
	glm::vec3 lightCol(1.f);
	float ambientStrength = 0.035f;

	bool showAxes = true;
	bool simpleWireframe = false;
	bool inDrawMode = true;
	bool showbounds = true;
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

	glm::vec3 lineColor{ 0.0f, 1.0f, 0.0f };
	std::vector<Line> lines;
	Line* lineInProgress = nullptr;
	float pointEpsilon = 0.01f;

	glm::vec3 boundColor{ 1.0f, 0.7f, 0.0f };
	std::vector<Line> bounds;
	Line* boundInProgress = nullptr;
	
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

	// RENDER LOOP
	while (!window.shouldClose()) {
		glfwPollEvents();

		// Line Drawing Logic. Max 2 lines can be drawn at a time
		if (inDrawMode && cb->leftMouseDown) {
			float perspectiveMultiplier = glm::tan(glm::radians(22.5f)) * cam.radius;
			glm::vec4 cursorPos = glm::vec4(cb->getCursorPosGL() * perspectiveMultiplier, -cam.radius, 1.0f);
			cursorPos = glm::inverse(cam.getView()) * cursorPos;

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
			}
			else if (lines.size() < 2){ 
				// create a new line
				lines.emplace_back(std::vector<Vertex>{ Vertex{ cursorPos, lineColor, glm::vec3(0.0f) } });
				lineInProgress = &lines.back();
				lineInProgress->updateGPU();
			}
		}
		else if (!inDrawMode || !cb->leftMouseDown)
			lineInProgress = nullptr;

		// Three functions that must be called each new frame.
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("CPSC 589 Project");

		bool change = false; // Whether any ImGui variable's changed.

		//change |= ImGui::ColorEdit3("Diffuse colour", glm::value_ptr(diffuseCol));

		// The rest of our ImGui widgets.
		//change |= ImGui::DragFloat3("Light's position", glm::value_ptr(lightPos));
		//change |= ImGui::ColorEdit3("Light's colour", glm::value_ptr(lightCol));
		//change |= ImGui::SliderFloat("Ambient strength", &ambientStrength, 0.0f, 1.f);
		change |= ImGui::Checkbox("Drawing Mode", &inDrawMode);
		change |= ImGui::Checkbox("Show Axes", &showAxes);
		change |= ImGui::Checkbox("Show Wireframe", &simpleWireframe);
		change |= ImGui::Checkbox("Show Bounds (for Debug)", &showbounds);

		if (inDrawMode) {
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
		}

		//if (meshes.size() > 0) {
		//	ImGui::SliderInt("Object Select", &meshchoice, 0, meshes.size());
		//}

		//if (lines.size() % 2 != 0 && lines.size() != 0) {
		//	if (ImGui::Button("Update Sweep") && meshchoice != 0) {
		//		meshes[meshchoice - 1].sweep = meshes[meshchoice - 1].cam.standardize(lines.back().verts);
		//		lines.pop_back();
		//		meshes[meshchoice - 1].create(75);
		//		meshes[meshchoice - 1].updateGPU();
		//		
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
				meshInProgress->bound2 = lines.back().verts;
				lines.pop_back();
				meshInProgress->sweep = cam.getcircle(50);
				meshInProgress->cam = cam;
				meshInProgress->create(75);
				meshInProgress->updateGPU();

				bounds.emplace_back();
				boundInProgress = &bounds.back();
				for (auto i = (meshInProgress->bound1).verts.begin(); i < (meshInProgress->bound1).verts.end(); i++) {
					boundInProgress->verts.push_back(Vertex{ (*i).position, glm::vec3(1.f, 0.7f, 0.f), (*i).normal });
				}
				boundInProgress->updateGPU();
				boundInProgress = nullptr;

				bounds.emplace_back();
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
			}
		}

		ImGui::Text("");
		ImGui::Text("Export to .obj");
		ImGui::InputText("Filename", ObjFilename, size_t(32));
		if (sizeof(ObjFilename) > 0 && ImGui::Button("Save"))
			exportToObj(ObjFilename, meshes);
		ImGui::Text("");

		// Framerate display, in case you need to debug performance.
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
		glPolygonMode(GL_FRONT_AND_BACK, (simpleWireframe ? GL_LINE : GL_FILL) );

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
			mesh.draw(lightingShader);
		}
		
		// Drawing Lines (no Lighting)
		noLightingShader.use();
		cb->viewPipeline();
		for (Line& line : lines) {
			line.draw(noLightingShader);
		}

		// Drawing Lines (no Lighting)
		if (showbounds) {
			for (Line& bound : bounds) {
				bound.draw(noLightingShader);
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
