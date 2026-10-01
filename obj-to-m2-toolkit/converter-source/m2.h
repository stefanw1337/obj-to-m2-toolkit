#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "vec3d.h"
#include "m2struct.h"

class m2
{
public:
	m2(void);
	~m2(void);

	bool loadFromFile(std::string path);
	void saveToFile(std::string path);

	void printTextures();
	void printSize();

	// Getters / Setters

	void setVersion(uint32 version) { m_version = version; };
	uint32 getVersion() { return m_version; };

	uint32 getVerticeCount() { return Vertices.size(); }
	uint32 getTriangleCount() { return Views.at(0).Triangles.size(); }

	inline void setName(const std::string& name) { m_name = name; };
	std::string getName() { return m_name; };

	// For now
	void ReserveVertices(int n);
	void AddVertice(Vec3D position, Vec3D normal, Vec2D uvmap, int index);

	// for ever
	void AddSkin(uint32 lod);
	void AddVertice(Vec3D position, Vec3D normal, Vec2D uvmap);
	void AddTriangle(triangle tri);
	void AddTexture(std::string name, uint32 type);
	void AddTextureUnit(uint16 submesh, uint16 renderflag, uint16 texture, bool reflect = false);
	void AddSubmesh(uint32 id, uint16 StartVertex, uint16 nVertices, uint16 StartTriangle, uint16 nTriangles);
	void AddRenderFlag(uint16 flag, uint16 blending);
	void AddBone(int32 keyBoneId, uint32 flags, Vec3D pivotPoint);
	void AddDummyAnim();
	void AddDummyTransparency();
	void UpdateCollision();
	void SetCollisionMesh(const std::vector<Vec3D>& vertices, const std::vector<triangle>& triangles);
	void FlipTex();

	//Get function
	skin* getSkins() { return Views.data(); }
	renderFlag* getRenderFlags() { return RenderFlags.data(); }
	texture* getTextures() { return Textures.data(); }

private:

	//Saving functions
	uint32 calcHeaderSize();
	uint32 calcAnimationsSize();
	uint32 calcBonesSize(bool withAnimationBlock = true, bool withAnimationBlockValues = true);
	uint32 calcVerticesSize();
	uint32 calcTexturesSize(bool withFilename = true);
	uint32 calcTransparencySize();

	template<class T>
	uint32 calcAnimationBlockSize(std::vector<std::vector<std::pair<uint32, T> > >  &storageMap, bool withValue = true);

	uint32 calcLookupTableSize(std::map<uint32, int16> &lookupMap) { return lookupMap.size() * sizeof(int16); };

	void fillM2Header(m2header &header);
	
	void writeAnimations(std::ofstream &stream);
	void writeBones(std::ofstream &stream);
	void writeBonesAblock(std::ofstream &stream);
	void writeVertices(std::ofstream &stream);
	void writeTextures(std::ofstream &stream);
	void writeRenderFlag(std::ofstream &stream);
	void writeTransparency(std::ofstream &stream);
	void writeBoundingData(std::ofstream &stream);

	void writeViews(std::string path);

	void writeLookupTable(std::ofstream &stream, std::map<uint32, int16> &lookupMap);

	template<class T>
	void writeAnimationSubBlock( std::ofstream &stream, uint32 offset, std::vector<std::vector<std::pair<uint32, T> > >  &storageMap, uint32 nPairs);


	//Loading functions
	void loadAnimations(std::ifstream &stream, uint32 offset, uint32 n);
	void loadBones(std::ifstream &stream, uint32 offset, uint32 n);
	void loadVertices(std::ifstream &stream, uint32 offset, uint32 n);
	void loadColors(std::ifstream &stream, uint32 offset, uint32 n);
	void loadTextures(std::ifstream &stream, uint32 offset, uint32 n);
	void loadTransparency(std::ifstream &stream, uint32 offset, uint32 n);
	void loadUVAnimation(std::ifstream &stream, uint32 offset, uint32 n);
	void loadTexReplace(std::ifstream &stream, uint32 offset, uint32 n);
	void loadRenderFlags(std::ifstream &stream, uint32 offset, uint32 n);
	void loadBoundingTriangles(std::ifstream &stream, uint32 offset, uint32 n);
	void loadBoundingVertices(std::ifstream &stream, uint32 offset, uint32 n);
	void loadBoundingNormals(std::ifstream &stream, uint32 offset, uint32 n);
	void loadAttachments(std::ifstream &stream, uint32 offset, uint32 n);
	void loadEvents(std::ifstream &stream, uint32 offset, uint32 n);
	void loadCameras(std::ifstream &stream, uint32 offset, uint32 n);
	void loadRibbonEmitters(std::ifstream &stream, uint32 offset, uint32 n);
	void loadParticleEmitters(std::ifstream &stream, uint32 offset, uint32 n);

	void loadLookupTable(std::ifstream &stream, uint32 offset, uint32 n, std::map<uint32, int16> &lookupMap);

	void loadViews(std::string filename, uint32 n);

	//Internal function for reading etc.. 
	void ReadString(std::string &string, std::ifstream &stream, uint32 offset, uint32 length);

	template<class T>
	void ReadAnimBlock(std::vector<std::vector<std::pair<uint32, T> > > &storageMap, std::ifstream &stream, m2ABlock &animblock);


	// 3D Calc function

	void calcVertexBox();
	void calcBoundingBox();

private:

	uint32 m_version;
	std::string m_name;
	GlobalModelFlag GlobalModelFlag; 

	std::vector<animation> Animations;
	std::vector<bone> Bones; // Todo check how ABlock / SubABlock works and how we can make it inside the bone struct
	std::vector<vertice> Vertices;
	std::vector<texture> Textures;
	std::vector<renderFlag> RenderFlags;
	std::vector<skin> Views;
	std::vector<transparency> Transparency;

	std::vector<Vec3D> BoundingVertices;
	std::vector<triangle> BoundingTriangles;
	std::vector<Vec3D> BoundingNormals;
	bool HasCustomCollision = false;
	bool HasDummyAnimation = false;

	Vec3D VertexBox[2];
	float VertexRadius;

	Vec3D BoundingBox[2];
	float BoundingRadius;

	// Lookup Tables 
	// TODO Put them somewhere else. We'll see if we can generate those automatically

	std::map<uint32, int16> AnimationLookupTable;
	std::map<uint32, int16> BoneLookupTable;
	std::map<uint32, int16> KeyBoneLookupTable;
	std::map<uint32, int16> TextureLookupTable;
	std::map<uint32, int16> TransLookupTable;
	std::map<uint32, int16> UVAnimLookupTable;
	std::map<uint32, int16> AttachLookupTable;
	std::map<uint32, int16> CameraLookupTable;
	std::map<uint32, int16> TextureUnitLookupTable;
	std::map<uint32, int16> ReplaceableTextureLookupTable;
};

