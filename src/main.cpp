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

// EXAMPLE CALLBACKS
class Callbacks3D : public CallbackInterface {

public:
	// Constructor. We use values of -1 for attributes that, at the start of
	// the program, have no meaningful/"true" value.
	Callbacks3D(ShaderProgram& lightingShader, ShaderProgram& noLightingShader, ShaderProgram& pickerShader, Camera& camera, int screenWidth, int screenHeight)
		: lightingShader(lightingShader)
		, noLightingShader(noLightingShader)
		, pickerShader(pickerShader)
		, camera(camera)
		, rightMouseDown(false)
		, leftMouseDown(false)
		, mouseOldX(-1.0)
		, mouseOldY(-1.0)
		, screenWidth(screenWidth)
		, screenHeight(screenHeight)
		, aspect(screenWidth/screenHeight)
	{
		updateUniformLocations();
	}

	void updateIDUniform(int idToRender) {
		// Like viewPipelinePicker(), this function assumes pickerShader.use() was called before.
		glUniform1i(pickerIDLoc, idToRender);
	}

	virtual void keyCallback(int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_R && action == GLFW_PRESS) {
			lightingShader.recompile();
			noLightingShader.recompile();
			pickerShader.recompile();
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

	void lightingViewPipeline() {
		glm::mat4 M = glm::mat4(1.0);
		glm::mat4 V = camera.getView();
		glm::mat4 P = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 1000.f);
		glUniformMatrix4fv(lightingMLoc, 1, GL_FALSE, glm::value_ptr(M));
		glUniformMatrix4fv(lightingVLoc, 1, GL_FALSE, glm::value_ptr(V));
		glUniformMatrix4fv(lightingPLoc, 1, GL_FALSE, glm::value_ptr(P));
	}

	void noLightingViewPipeline() {
		glm::mat4 M = glm::mat4(1.0);
		glm::mat4 V = camera.getView();
		glm::mat4 P = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 1000.f);
		glUniformMatrix4fv(noLightingMLoc, 1, GL_FALSE, glm::value_ptr(M));
		glUniformMatrix4fv(noLightingVLoc, 1, GL_FALSE, glm::value_ptr(V));
		glUniformMatrix4fv(noLightingPLoc, 1, GL_FALSE, glm::value_ptr(P));
	}

	void viewPipelinePicker() {
		glm::mat4 M = glm::mat4(1.0);
		glm::mat4 V = camera.getView();
		glm::mat4 P = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 1000.f);

		glUniformMatrix4fv(mLocPicker, 1, GL_FALSE, glm::value_ptr(M));
		glUniformMatrix4fv(vLocPicker, 1, GL_FALSE, glm::value_ptr(V));
		glUniformMatrix4fv(pLocPicker, 1, GL_FALSE, glm::value_ptr(P));
	}

	void updateShadingUniforms(
		const glm::vec3& lightPos, float diffuseConstant, float ambientStrength
	)
	{
		// Like viewPipeline(), this function assumes shader.use() was called before.
		glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
		glUniform1f(diffuseConstantLoc, diffuseConstant);
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

	glm::vec2 cursorPosScreenCoords() {
		return glm::vec2(mouseOldX, mouseOldY);
	}

	bool rightMouseDown;
	bool leftMouseDown;

private:
	// Uniform locations do not, ordinarily, change between frames.
	// However, we may need to update them if the shader is changed and recompiled.
	void updateUniformLocations() {
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

	ShaderProgram& lightingShader;
	ShaderProgram& noLightingShader;
	ShaderProgram& pickerShader;
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

int findSelectedObjectIndex(
	Framebuffer& pickerFB, 
	Texture& pickerTex, 
	ShaderProgram& pickerShader, 
	std::shared_ptr<Callbacks3D> cb, 
	Window& window, 
	std::vector<Mesh>& meshes
) {
	int pickerClearValue[4] = { 0, 0, 0, 0 };

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
	for (int i = 0; i < meshes.size(); i++) {
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

int main() {
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
	//bool sweep = false;

	// Set the initial, default values of the shading uniforms.
	lightingShader.use();
	cb->updateShadingUniforms(lightPos, diffuseConstant, ambientStrength);

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
	std::string lastExportedFilename = "";

	int hoveredObjectIndex = -1;
	int selectedObjectIndex = -1;

	enum ViewType { FREE_VIEW, DRAW_VIEW, OBJECT_VIEW };
	ViewType view = FREE_VIEW;

	// RENDER LOOP
	while (!window.shouldClose()) {
		glfwPollEvents();

		// Detect Hovered Objects in FREE_VIEW
		if (view == FREE_VIEW) hoveredObjectIndex = findSelectedObjectIndex(pickerFB, pickerTex, pickerShader, cb, window, meshes);
		else hoveredObjectIndex = -1;

		// Line Drawing Logic. Max 2 lines can be drawn at a time. Only in DRAW_VIEW
		if (view == DRAW_VIEW && cb->leftMouseDown) {
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
		else
			lineInProgress = nullptr;

		// Start ImGui Frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		bool change = false; // Whether any ImGui variable's changed.

		if (view == FREE_VIEW) {
			ImGui::Begin("Free View");
			if (ImGui::Button("Draw Mode")) {
				view = DRAW_VIEW;
				change = true;
			}

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
		else if (view == DRAW_VIEW) {
			ImGui::Begin("Draw Mode");
			if (ImGui::Button("Free View")) {
				view = FREE_VIEW;
				change = true;
				lines.clear();
			}

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
		}
		else if (view == OBJECT_VIEW) {
				ImGui::Begin("Object View - Object");
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

		// Framerate display, in case you need to debug performance.
		ImGui::Text("");
		change |= ImGui::Checkbox("Show Axes", &showAxes);
		ImGui::Text("Average %.1f ms/frame (%.1f fps)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
		ImGui::Render();

		if (change) {
			if (view == DRAW_VIEW) {
				cam.phi = 0.f;
				cam.theta = 0.f;
				cam.fix();
			}
			else if (view == FREE_VIEW) {
				cam.unFix();
			}
		}

		// RENDERING
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_FRAMEBUFFER_SRGB);
		glEnable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, (simpleWireframe ? GL_LINE : GL_FILL) );
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Drawing Axis Lines (no Lighting)
		if (showAxes) {
			noLightingShader.use();
			cb->noLightingViewPipeline();
			for (Line& line : axisLines) {
				line.draw();
			}
		}

		// Drawing Meshes (with Lighting)
		lightingShader.use();
		cb->lightingViewPipeline();
		for (int i = 0; i < meshes.size(); i++) {
			float a = ambientStrength;
			float d = diffuseConstant;
			if (hoveredObjectIndex >= 0 && i == hoveredObjectIndex) {
				d += 0.2f;
				a += 0.05f;
			}
			else if (hoveredObjectIndex >= 0 && i != hoveredObjectIndex) {
				d -= 0.2;
				a -= 0.05f;
			}
			cb->updateShadingUniforms(lightPos, d, a);
			meshes[i].draw();
		}
		
		// Drawing Lines (no Lighting)
		noLightingShader.use();
		cb->noLightingViewPipeline();
		for (Line& line : lines) {
			line.draw();
		}

		// Drawing Lines (no Lighting)
		if (showbounds) {
			noLightingShader.use();
			cb->noLightingViewPipeline();
			for (Line& bound : bounds) {
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
