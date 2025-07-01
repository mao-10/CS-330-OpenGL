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
/**************************************************************
*LoadSceneTextures()
* 
* This method is for preparing the 3d scene by loading the
* shapes and textures in memory
***************************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;
	// get textures
	bReturn = CreateGLTexture(
		"textures/black_marble.jpg", "box"
	);
	bReturn = CreateGLTexture(
		"textures/cement.jpg", "potBody"
	);
	bReturn = CreateGLTexture(
		"textures/cement.jpg", "potRim"
	);
	bReturn = CreateGLTexture(
		"textures/cement.jpg", "potSphereBottom"
	);
	bReturn = CreateGLTexture(
		"textures/dirt.jpg", "potDirt"
	);
	bReturn = CreateGLTexture(
		"textures/bricks_white_seamless.jpg", "backsplash"
	);
	bReturn = CreateGLTexture(
		"textures/marble_light_seamless.jpg", "counter"
	);
	bReturn = CreateGLTexture(
		"textures/green_texture.jpg", "stem"
	);
	bReturn = CreateGLTexture(
		"textures/green_texture.jpg", "leaf"
	);
	bReturn = CreateGLTexture(
		"textures/metal.jpg", "metal"
	);
	bReturn = CreateGLTexture(
		"textures/plastic_dark_seamless.png", "plastic"
	);

	// after the texture image data is loaded into memory
	// the loaded textures need to be bound to texture slots
	// there are a total of 16 available slots
	BindGLTextures();
}
/***********************************************************
* DefineObjectMaterials()
*
* This method is for configuring the different materials 
* for all the objects in the scene
************************************************************/
void SceneManager::DefineObjectMaterials()
{
	// for plant pot
	OBJECT_MATERIAL cementMaterial;
	cementMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cementMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	cementMaterial.shininess = 0.5;
	cementMaterial.tag = "cement";

	m_objectMaterials.push_back(cementMaterial);

	// for backdrop
	OBJECT_MATERIAL tileMaterial;
	tileMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	tileMaterial.specularColor = glm::vec3(0.4f, 0.5f, 0.6f);
	tileMaterial.shininess = 25.0;
	tileMaterial.tag = "tile";

	m_objectMaterials.push_back(tileMaterial);

	// for black tray and countertop
	OBJECT_MATERIAL marbleMaterial;
	marbleMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f);
	marbleMaterial.specularColor = glm::vec3(0.3f, 0.4f, 0.4f);
	marbleMaterial.shininess = 28.0;
	marbleMaterial.tag = "marble";

	m_objectMaterials.push_back(marbleMaterial);

	// for dirt
	OBJECT_MATERIAL dirtMaterial;
	dirtMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	dirtMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	dirtMaterial.shininess = 0.3;
	dirtMaterial.tag = "dirt";

	m_objectMaterials.push_back(dirtMaterial);

	// for metal
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.diffuseColor = glm::vec3(0.5f, 0.4f, 0.4f);
	metalMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	metalMaterial.shininess = 24.0;
	metalMaterial.tag = "metal";

	m_objectMaterials.push_back(metalMaterial);

	// for glass
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.9f, 0.9f, 0.9f);
	glassMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glassMaterial.shininess = 95.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);
	// for plastic
	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.diffuseColor = glm::vec3(0.9f, 0.9f, 0.9f);
	plasticMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	plasticMaterial.shininess = 20.0;
	plasticMaterial.tag = "plastic";

	m_objectMaterials.push_back(plasticMaterial);

}

/***********************************************************
* SetupSceneLights()
*
* This method is called to add and configure the light
* sources for the scene... There are upto 4 light sources
************************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is needed for telling the shaders
	// to render the 3D scene with custom lighting
	// to use default, comment next line out
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// directional light to emulate sunlight
	m_pShaderManager->setVec3Value("directionalLight.direction", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.52f, 0.56f, 0.5f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// point light 1
	m_pShaderManager->setVec3Value("pointLights[0].position", -4.0f, 8.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// point light 2
	m_pShaderManager->setVec3Value("pointLights[1].position", 4.0f, 8.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	// point light 3
	m_pShaderManager->setVec3Value("pointLights[2].position", 3.8f, 5.5f, 4.0f);
	m_pShaderManager->setVec3Value("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("pointLights[2].specular", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);

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
	// load texture files into scene
	LoadSceneTextures();
	// load materials
	DefineObjectMaterials();
	// load lights
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	// load shapes for scene
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("counter");
	SetShaderMaterial("marble");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	// backdrop/background plane for the kitchen tile/wall
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 00.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 10.0f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("backsplash");
	SetShaderMaterial("tile");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/******************************************************************************/

	// load box for tray that sits under items
	scaleXYZ = glm::vec3(15.0f, 1.0f, 9.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the postion for the mesh
	positionXYZ = glm::vec3(3.0f, 1.0f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	// set texture
	SetShaderTexture("box");
	SetShaderMaterial("marble");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/*****************************************************************/
	// render a half-sphere for the bottom of the pot?
	scaleXYZ = glm::vec3(3.0f, 2.0f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, 3.0f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.9, 0.9, 0.9, 1);
	SetShaderTexture("potSphereBottom");
	SetShaderMaterial("cement");
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	/*****************************************************************/
	// render a cylinder for the body of the pot
	scaleXYZ = glm::vec3(3.0f, 4.0f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, 3.0f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.9, 0.9, 0.9, 1);
	SetShaderTexture("potDirt");
	SetShaderMaterial("dirt");
	m_basicMeshes->DrawCylinderMesh(true,false, false);
	// set texture
	SetShaderTexture("potBody");
	SetShaderMaterial("cement");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(false,true,true);

	/*****************************************************************/
	// render a torus for the rim
	scaleXYZ = glm::vec3(2.5f, 2.6f, 2.0f);

	// set the XYZ rotation for the mesh
	// rotate so torus sits on top of cylinder
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, 7.0f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.9, 0.9, 0.9, 1);
	//set texture
	SetShaderTexture("potRim");
	SetShaderMaterial("cement");
	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/*****************************************************************/
	// render a cylinder for a plant stem
	scaleXYZ = glm::vec3(0.1f, 8.0f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, 2.0f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("stem");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/***********************************************************************/

	// render a cylinder for a plant stem
	scaleXYZ = glm::vec3(0.1f, 8.0f, 0.1f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 10.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, 2.0f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	// Add texture for stem
	SetShaderTexture("stem");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/***********************************************************************/

	// render a cylinder for a plant stem
	scaleXYZ = glm::vec3(0.1f, 8.0f, 0.1f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 10.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.5f, 2.0f, -4.1f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	// Add texture for stem
	SetShaderTexture("stem");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************************/

	// render a cylinder for a plant stem
	scaleXYZ = glm::vec3(0.1f, 9.0f, 0.1f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 10.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.8f, 2.0f, -3.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// add texture for stem
	SetShaderTexture("stem");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/************************************************************/
	// render a cylinder for a plant stem
	scaleXYZ = glm::vec3(0.1f, 8.0f, 0.1f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 10.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.3f, 2.0f, -3.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// add texture for stem
	SetShaderTexture("stem");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/***************************************************************/
	// render a half sphere for a leaf
	scaleXYZ = glm::vec3(0.3f, 0.05f, 0.1f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.8f, 7.8f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// add texture for leaf
	SetShaderTexture("leaf");

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();

	/************************************************************/
	/***************************************************************/
	// render a half sphere for a leaf
	scaleXYZ = glm::vec3(0.3f, 0.05f, 0.1f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.75f, 9.1f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("leaf");

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();

	/***************************************************************/
	// render a half sphere for a leaf
	scaleXYZ = glm::vec3(0.25f, 0.05f, 0.1f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.8f, 8.3f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("leaf");

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();

	/***************************************************************/
	// render a half sphere for a leaf
	scaleXYZ = glm::vec3(0.3f, 0.05f, 0.1f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.8f, 9.9f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set shader texture
	SetShaderTexture("leaf");

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();

	/***************************************************************/
	// render a half sphere for a leaf
	scaleXYZ = glm::vec3(0.25f, 0.05f, 0.1f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 90.0f;
	YrotationDegrees = 1.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.2f, 9.8f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set shader texture
	SetShaderTexture("leaf");

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();
	/***************************************************************/
	// render a half sphere for a leaf
	scaleXYZ = glm::vec3(0.3f, 0.05f, 0.1f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 90.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 4.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.25f, 8.1f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set texture
	//SetShaderColor(0.4, 0.4, 0.3, 1);
	SetShaderTexture("leaf");

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();

	/************************************************************/
	// render a half sphere for a leaf
	scaleXYZ = glm::vec3(0.25f, 0.1f, 0.1f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 90.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 4.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.8f, 8.9f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set texture
	//SetShaderColor(0.4, 0.4, 0.3, 1);
	SetShaderTexture("leaf");

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();
	/************************************************************/
	// render a half sphere for a leaf
	scaleXYZ = glm::vec3(0.2f, 0.05f, 0.1f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 4.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.25f, 8.5f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set texture
	//SetShaderColor(0.4, 0.4, 0.3, 1);
	SetShaderTexture("leaf");

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();
	/************************************************************/
	// render a box for the clock
	scaleXYZ = glm::vec3(3.0f, 3.0f, 3.0f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 3.0f, -3.07f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the back
	SetShaderTexture("plastic");
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::back);
	// draw the top
	SetShaderTexture("plastic");
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::top);
	// draw the bottom
	SetShaderTexture("plastic");
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::bottom);
	// draw the left side
	SetShaderTexture("plastic");
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::left);
	// draw the right side of box
	SetShaderTexture("plastic");
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::right);
	
	/*************************************************************/
	// render a box for the clock
	scaleXYZ = glm::vec3(3.0f, 3.0f, 3.0f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 3.0f, -3.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::front);

	SetShaderTexture("plastic");
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/**********************************************************/
	// render a half sphere for the clock
	scaleXYZ = glm::vec3(0.2f, 0.1f, 0.2f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 3.0f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("plastic");
	m_basicMeshes->DrawHalfSphereMesh();
	/*************************************************************/
	// render a cylinder for the clock
	scaleXYZ = glm::vec3(0.05f, 1.2f, 0.05f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 3.1f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("plastic");
	m_basicMeshes->DrawCylinderMesh();

	/*************************************************************/
	// render a cylinder for the clock
	scaleXYZ = glm::vec3(0.05f, 1.1f, 0.05f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 3.0f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("plastic");
	m_basicMeshes->DrawCylinderMesh();
	/*************************************************************/
	// render a cylinder for the salt shaker
	scaleXYZ = glm::vec3(0.5f, 1.0f, 0.5f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.7f, 1.5f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 0.3);
	SetShaderMaterial("glass");
	m_basicMeshes->DrawCylinderMesh();

	/*************************************************************/
	// render a tapered cylinder for the salt shaker
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.7f, 2.5f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 0.3);
	SetShaderMaterial("glass");
	m_basicMeshes->DrawTaperedCylinderMesh();

	/*************************************************************/
	// render a cylinder for the top of salt shaker
	scaleXYZ = glm::vec3(0.3f, 0.2f, 0.2f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.7f, 3.0f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("metal");
	SetShaderMaterial("metal");
	m_basicMeshes->DrawCylinderMesh();

	/*************************************************************/
	// render a half sphere for the top of salt shaker
	scaleXYZ = glm::vec3(0.3f, 0.2f, 0.2f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.7f, 3.2f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("metal");
	SetShaderMaterial("metal");
	m_basicMeshes->DrawHalfSphereMesh();

	/*************************************************************/
	// render a cylinder for the salt shaker
	scaleXYZ = glm::vec3(0.5f, 1.0f, 0.5f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(9.9f, 1.5f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 0.3);
	SetShaderMaterial("glass");
	m_basicMeshes->DrawCylinderMesh();

	/*************************************************************/
	// render a tapered cylinder for the salt shaker
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(9.9f, 2.5f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 0.3);
	SetShaderMaterial("glass");
	m_basicMeshes->DrawTaperedCylinderMesh();

	/*************************************************************/
	// render a cylinder for the top of salt shaker
	scaleXYZ = glm::vec3(0.3f, 0.2f, 0.2f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(9.9f, 3.0f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("metal");
	SetShaderMaterial("metal");
	m_basicMeshes->DrawCylinderMesh();

	/*************************************************************/
	// render a half sphere for the top of salt shaker
	scaleXYZ = glm::vec3(0.3f, 0.2f, 0.2f);

	// set the XYZ rotation for the mesh
	// rotate stem
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(9.9f, 3.2f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("metal");
	SetShaderMaterial("metal");
	m_basicMeshes->DrawHalfSphereMesh();
}
