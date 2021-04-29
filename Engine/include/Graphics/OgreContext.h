#pragma once

#ifndef _GRAPHICS_OGRECONT_H
#define _GRAPHICS_OGRECONT_H

#include <string>
#ifdef _DEBUG
#endif
class RTShaderTecnhiqueResolveListener;

namespace Ogre {
	class Root;
	class SceneManager;
	class Camera;
	class SceneNode;
	class FileSystemLayer;
	class Plane;

	namespace RTShader {
		class ShaderGenerator;
	}
}

class OgreContext
{
private:
	static OgreContext* instance_;
	Ogre::Root* ogreRoot_ = nullptr;
	Ogre::SceneManager* ogreSceneManager_ = nullptr;
	Ogre::RTShader::ShaderGenerator* mShaderGenerator_;
	Ogre::FileSystemLayer* mFSLayer_;
	RTShaderTecnhiqueResolveListener* mMaterialListener_;
	std::string appName_;


	OgreContext(const std::string& appName);
	void createRoot();
	void createSceneManager();
	void init();
	void loadFromResourceFile();
public:
	~OgreContext();
	Ogre::Root* getOgreRoot();
	Ogre::Root* getOgreRoot() const;
	Ogre::SceneManager* getSceneManager();
	Ogre::SceneManager* getSceneManager() const;
	
	static OgreContext* getInstance();
	static bool setupInstance(const std::string& appName);
	void setupRTShaderGenerator();
	static void clean();
	//Hay que poner en el material un lighting off
	void setSkyPlane(const std::string& materialName, const Ogre::Plane& plane, int width, int height, float bow);
};

#endif