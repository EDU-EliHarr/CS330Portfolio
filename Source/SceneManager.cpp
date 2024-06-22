///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

#include <GLFW/glfw3.h>

// Global variables for camera control
glm::vec3 cameraPos = glm::vec3(5.0f, 5.0f, 10.0f); 
glm::vec3 cameraFront = glm::vec3(-0.5f, -0.5f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraRight; // Calculated Value
float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame
float yaw = -90.0f;     
float pitch = 0.0f;
float lastX = 400, lastY = 300;
float movementSpeed = 2.5f; // Initial movement speed
bool firstMouse = true;
enum ProjectionMode { PERSPECTIVE, ORTHOGRAPHIC };
ProjectionMode currentProjectionMode = PERSPECTIVE;


// Constants for orthographic camera position
const glm::vec3 ORTHO_CAMERA_POS = glm::vec3(0.0f, 0.0f, 10.0f);
const glm::vec3 ORTHO_CAMERA_FRONT = glm::vec3(0.0f, 0.0f, -1.0f);
const glm::vec3 ORTHO_CAMERA_UP = glm::vec3(0.0f, 1.0f, 0.0f);

//Forward Declarations (to resolve build errors)
void mouse_callback(double xpos, double ypos);
void scroll_callback(double xoffset, double yoffset);
void ProcessInput();

void SceneManager::SetupSceneLights()
{
	// Enable custom lighting in the shaders
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	m_pShaderManager->setVec3Value("globalAmbientColor", 0.09f, 0.09f, 0.06f); //slight yellow overall so the blue from the monitor stands out

	// Overhead light (white light)
	m_pShaderManager->setVec3Value("lightSources[0].position", 0.0f, 7.0f, 3.0f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 64.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.15f);

	// Monitor light (directional)
	m_pShaderManager->setVec3Value("lightSources[1].position", 0.0f, .5f, -1.3f); // Slightly in front of the screen
	m_pShaderManager->setVec3Value("lightSources[1].direction", 0.0f, -0.5f, 1.0f); // Direction towards the front
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.5f, 0.5f, 5.0f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.5f, 0.5f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.01f);
}


void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL satinMaterial;
	satinMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	satinMaterial.ambientStrength = 0.3f;
	satinMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
	satinMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	satinMaterial.shininess = 22.0f; // Moderate shininess for a satin finish
	satinMaterial.tag = "satin";
	m_objectMaterials.push_back(satinMaterial);

	OBJECT_MATERIAL monitorMaterial;
	monitorMaterial.ambientColor = glm::vec3(0.8f, 0.8f, 10.0f); // Very emissive
	monitorMaterial.ambientStrength = 1.0f; // Maximum ambient strength
	monitorMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 1.0f); // Very bright blue light
	monitorMaterial.specularColor = glm::vec3(0.5f, 0.5f, 1.0f); // Very bright specular
	monitorMaterial.shininess = 60.0f; // Higher shininess for more reflection
	monitorMaterial.tag = "monitor";
	m_objectMaterials.push_back(monitorMaterial);

	OBJECT_MATERIAL greenMaterial;
	greenMaterial.ambientColor = glm::vec3(0.0f, 3.0f, 0.0f); // Bright green
	greenMaterial.ambientStrength = 1.0f;
	greenMaterial.diffuseColor = glm::vec3(0.0f, 3.0f, 0.0f);
	greenMaterial.specularColor = glm::vec3(0.0f, 3.0f, 0.0f);
	greenMaterial.shininess = 1.0f;
	greenMaterial.tag = "green";
	m_objectMaterials.push_back(greenMaterial);

}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	//Load the plane mesh for the desk
	m_basicMeshes->LoadPlaneMesh();
	
	// Load the box mesh for the keyboard, monitor, and PC tower
	m_basicMeshes->LoadBoxMesh();
	
	// Load the cylinder mesh for the mouse
	m_basicMeshes->LoadCylinderMesh();

	// Load the torus mesh for the power button
	m_basicMeshes->LoadTorusMesh();
	
	// Set up input callbacks
	glfwSetCursorPosCallback(glfwGetCurrentContext(), [](GLFWwindow*, double xpos, double ypos) { mouse_callback(xpos, ypos); });
	glfwSetScrollCallback(glfwGetCurrentContext(), [](GLFWwindow*, double xoffset, double yoffset) { scroll_callback(xoffset, yoffset); });

	// Load desk texture
	CreateGLTexture("textures/desk.jpg", "desk");

	// Load monitor texture
	CreateGLTexture("textures/monitor.jpg", "monitor");

	// Load keyboard texture
	CreateGLTexture("textures/keyboard.jpg", "keyboard");

	// Load mouse texture
	CreateGLTexture("textures/mouse.jpg", "mouse");

	// Load PC tower texture
	CreateGLTexture("textures/pc_tower.jpg", "pc_tower");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();

	// Define the materials
	DefineObjectMaterials();

	// Setup the scene lights
	SetupSceneLights();
}

void ProcessInput() {
	cameraRight = glm::normalize(glm::cross(cameraFront, cameraUp)); // Calculate the right vector
	float cameraSpeed = movementSpeed * deltaTime; // Adjust speed based on deltaTime

	if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_W) == GLFW_PRESS) {
		cameraPos += cameraSpeed * cameraFront;
	}
	if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_S) == GLFW_PRESS) {
		cameraPos -= cameraSpeed * cameraFront;
	}
	if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_A) == GLFW_PRESS) {
		cameraPos -= cameraSpeed * cameraRight;
	}
	if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_D) == GLFW_PRESS) {
		cameraPos += cameraSpeed * cameraRight;
	}
	if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_Q) == GLFW_PRESS) {
		cameraPos += cameraSpeed * cameraUp;
	}
	if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_E) == GLFW_PRESS) {
		cameraPos -= cameraSpeed * cameraUp;
	}

	// Handle projection mode switching
	if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_P) == GLFW_PRESS) {
		currentProjectionMode = PERSPECTIVE;
	}
	if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_O) == GLFW_PRESS) {
		currentProjectionMode = ORTHOGRAPHIC;
	}
}

void mouse_callback(double xpos, double ypos) {
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f; 
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	// Make sure that when pitch is out of bounds, screen doesn't get flipped
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);
}

void scroll_callback(double xoffset, double yoffset) {
	movementSpeed += yoffset; // Adjust the movement speed based on scroll input
	if (movementSpeed < 1.0f) // Prevent the speed from becoming negative or too slow
		movementSpeed = 1.0f;
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// Update deltaTime
	float currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	// Process keyboard input for camera movement
	ProcessInput();

	// Calculate view matrix based on current projection mode
	glm::mat4 view;
	if (currentProjectionMode == PERSPECTIVE) {
		view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	}
	else {
		// Set orthographic view to look directly along the z-axis
		view = glm::lookAt(ORTHO_CAMERA_POS, ORTHO_CAMERA_POS + ORTHO_CAMERA_FRONT, ORTHO_CAMERA_UP);
	}

	// Calculate projection matrix based on current projection mode
	glm::mat4 projection;
	if (currentProjectionMode == PERSPECTIVE) {
		projection = glm::perspective(glm::radians(45.0f), (float)800 / (float)600, 0.1f, 100.0f);
	}
	else {
		float left = -5.0f, right = 5.0f, bottom = -5.0f, top = 5.0f, zNear = 0.1f, zFar = 100.0f;
		projection = glm::ortho(left, right, bottom, top, zNear, zFar);
	}
	
	
	glm::mat4 model = glm::mat4(1.0f);

	// Set shader uniforms for view and projection
	m_pShaderManager->setMat4Value("view", view);
	m_pShaderManager->setMat4Value("projection", projection);
	m_pShaderManager->setVec3Value("viewPosition", cameraPos);

	// Apply transformations
	glm::vec3 scaleXYZ = glm::vec3(5.0f, 1.0f, 3.0f);
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);


	//Render the Scene


	// Render the desk

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("satin");
	SetShaderTexture("desk");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	// Render the monitor

	// Screen
	scaleXYZ = glm::vec3(2.0f, 1.2f, 0.1f);
	XrotationDegrees = -5.0f; // Tilt the screen back by 5 degrees
	positionXYZ = glm::vec3(0.0f, 1.1f, -1.75f);

	SetTransformations(scaleXYZ, XrotationDegrees, 0.0f, 0.0f, positionXYZ);
	SetShaderMaterial("monitor");
	SetShaderTexture("monitor");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Body
	scaleXYZ = glm::vec3(2.1f, 1.3f, 0.3f);
	XrotationDegrees = -5.0f; 
	positionXYZ = glm::vec3(0.0f, 1.1f, -1.9f);

	SetTransformations(scaleXYZ, XrotationDegrees, 0.0f, 0.0f, positionXYZ);
	SetShaderMaterial("satin");
	SetShaderTexture("pc_tower");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Stand
	scaleXYZ = glm::vec3(0.3f, 1.0f, 0.25f);
	XrotationDegrees = 0.0f; 
	positionXYZ = glm::vec3(0.0f, 0.5f, -1.9f);

	SetTransformations(scaleXYZ, XrotationDegrees, 0.0f, 0.0f, positionXYZ);
	SetShaderMaterial("satin");
	SetShaderTexture("pc_tower");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();


	// Render the keyboard
	//keys
	scaleXYZ = glm::vec3(2.4f, 0.2f, 1.0f);
	positionXYZ = glm::vec3(0.0f, 0.09f, -1.0f);
	
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("satin");
	SetShaderTexture("keyboard");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	//body
	scaleXYZ = glm::vec3(2.5f, 0.15f, 1.1f);
	positionXYZ = glm::vec3(0.0f, 0.1f, -1.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("satin");
	SetShaderTexture("pc_tower");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Render the mouse
	scaleXYZ = glm::vec3(0.3f, 0.1f, 0.4f);
	positionXYZ = glm::vec3(1.5f, 0.0f, 0.5f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("satin");
	SetShaderTexture("mouse");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Render the PC tower
	scaleXYZ = glm::vec3(1.0f, 2.5f, 1.5f);
	positionXYZ = glm::vec3(3.0f, 1.26f, -0.5f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("satin");
	SetShaderTexture("pc_tower");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Render the power button
	scaleXYZ = glm::vec3(0.1f, 0.1f, 0.1f); // Torus size
	positionXYZ = glm::vec3(2.7f, 2.0f, 0.25f); // Position on the front of the PC tower
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("mouse");
	SetShaderMaterial("green");
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/
}
