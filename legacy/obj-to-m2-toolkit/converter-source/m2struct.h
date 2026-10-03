#pragma once

#include <memory>
#include <string>
#include <vector>

#include "vec3d.h"
#include <vector>

typedef unsigned char uint8;
typedef char int8;
typedef unsigned __int16 uint16;
typedef __int16 int16;
typedef unsigned __int32 uint32;
typedef __int32 int32;

typedef std::pair< uint32, short> ShortTimeline;
typedef std::pair< uint32 /*timestamp*/,  Vec3D /* animFloat */> Vec3DTimeLine;
typedef std::pair< uint32 /*timestamp*/, Short4Vector /* quaternion */> ShortQuatTimeLine;

enum GlobalModelFlag
{
	Normal = 0,
	Tilt_x = 1,
	Tilt_y = 2,
	Unknown = 4,
	New_Field_Header = 8,
	Unknown2 = 16
};

/*
struct Short4Vector
{
	short x;
	short y;
	short z;
	short w;
};*/

// M2 Specific stuff ( with pointers / annoying stuff )

struct m2header {
	char id[4];
	uint32 version;
	uint32 nameLength;
	uint32 nameOfs;
	uint32 Flags;

	uint32 nGlobalSequences;
	uint32 ofsGlobalSequences;
	uint32 nAnimations;
	uint32 ofsAnimations;
	uint32 nAnimationLookup;
	uint32 ofsAnimationLookup;
	uint32 nBones;
	uint32 ofsBones;
	uint32 nKeyBoneLookup;
	uint32 ofsKeyBoneLookup;

	uint32 nVertices;
	uint32 ofsVertices;
	uint32 nViews;

	uint32 nColors;
	uint32 ofsColors;

	uint32 nTextures;
	uint32 ofsTextures;

	uint32 nTransparency; // H
	uint32 ofsTransparency;
	uint32 nTexAnims; // J
	uint32 ofsTexAnims;
	uint32 nTexReplace;
	uint32 ofsTexReplace;

	uint32 nRenderFlags;
	uint32 ofsRenderFlags;
	uint32 nBoneLookupTable;
	uint32 ofsBoneLookupTable;

	uint32 nTexLookup;
	uint32 ofsTexLookup;

	uint32 nTexUnitLookup; // L
	uint32 ofsTexUnitLookup;
	uint32 nTransparencyLookup; // M
	uint32 ofsTransparencyLookup;
	uint32 nTexAnimLookup;
	uint32 ofsTexAnimLookup;

	//not sure about these :/
	Vec3D VertexBoxMin;//?
	Vec3D VertexBoxMax;//?
	float VertexBoxRadius;
	Vec3D BoundingBoxMin;//?
	Vec3D BoundingBoxMax;//?
	float BoundingBoxRadius;

	uint32 nBoundingTriangles;
	uint32 ofsBoundingTriangles;
	uint32 nBoundingVertices;
	uint32 ofsBoundingVertices;
	uint32 nBoundingNormals;
	uint32 ofsBoundingNormals;

	uint32 nAttachments; // O
	uint32 ofsAttachments;
	uint32 nAttachLookup; // P
	uint32 ofsAttachLookup;
	uint32 nAttachments_2; // Q
	uint32 ofsAttachments_2;
	uint32 nLights; // R
	uint32 ofsLights;
	uint32 nCameras; // S
	uint32 ofsCameras;
	uint32 nCameraLookup;
	uint32 ofsCameraLookup;
	uint32 nRibbonEmitters; // U
	uint32 ofsRibbonEmitters;
	uint32 nParticleEmitters; // V
	uint32 ofsParticleEmitters;

};

struct m2ABlock
{
	uint16 InterpolationType;
	uint16 GlobalSequenceID;
	uint32 numberOfTimestampPairs;
	uint32 offsetToTimestampPairs;
	uint32 numberOfKeyFramePairs;
	uint32 offsetToKeyFramePairs;
};

struct m2SubABlock
{
	int32 nValues;
	int32 ofsValues;
};

struct m2bone
{
	int32 KeyBoneId;
	uint32 Flags;
	int16 ParentBone;
	uint16 Unknown[3];
	m2ABlock Translation;
	m2ABlock Rotation;
	m2ABlock Scaling;
	Vec3D PivotPoint;
};

struct m2transparency
{
	m2ABlock Alpha;
};

struct m2texture
{
	uint32 Type;
	uint32 Flags;
	uint32 lenFilename;
	uint32 ofsFilename;
};

// Stuff that can be used in the class and not only for reading / writing
struct bone
{
	int32 KeyBoneId;
	uint32 Flags;
	int16 ParentBone;
	uint16 Unknown[3];
	std::vector<std::vector<Vec3DTimeLine> > Translation;
	std::vector<std::vector<ShortQuatTimeLine> > Rotation;
	std::vector<std::vector<Vec3DTimeLine> > Scaling;
	Vec3D PivotPoint;

	uint16 TranslationInterp;
	uint16 TranslationGlobalSequenceID;
	uint16 RotationInterp;
	uint16 RotationGlobalSequenceID;
	uint16 ScalingInterp;
	uint16 ScalingGlobalSequenceID;
};

struct transparency
{
	std::vector<std::vector<ShortTimeline> > Alpha;
};

struct animation
{
	uint16 AnimationID;
	uint16 SubAnimationID;
	uint32 Length;
	float MovingSpeed;
	uint32 Flags;
	int16 Probability;
	uint16 Unknown0;
	uint32 Unknown1;
	uint32 Unknown2;
	uint32 PlaybackSpeed;
	Vec3D MinimumExtent;
	Vec3D MaximumExtent;
	float BoundRadius;
	int16 NextAnimation;
	uint16 Index;
};

struct renderFlag
{
	uint16 Flags;
	uint16 Blending;
};

struct vertice
{
	Vec3D Position;
	int8 BoneWeight[4];
	uint8 BoneIndices[4];
	Vec3D Normal;
	Vec2D TextureCoords;
	Vec2D Unknown;

	bool operator==(vertice lhs)
	{
		return (Position == lhs.Position && Normal == lhs.Normal && TextureCoords == lhs.TextureCoords);
	}

};

struct texture
{
	uint32 Type;
	uint32 Flags;
	std::string Filename;
};

// Skin struct

struct skinheader
{
	char id[4];
	uint32 nIndices;
	uint32 ofsIndices;
	uint32 nTriangles;
	uint32 ofsTriangles;
	uint32 nProperties;
	uint32 ofsProperties;
	uint32 nSubmeshes;
	uint32 ofsSubmeshes;
	uint32 nTextureUnits;
	uint32 ofsTextureUnits;
	uint32 LOD;
};

struct triangle
{
	uint16 indice1;
	uint16 indice2;
	uint16 indice3;
};

struct vertexProperty
{
	uint8 boneIndice1;
	uint8 boneIndice2;
	uint8 boneIndice3;
	uint8 boneIndice4;
};

struct submesh
{
	uint32 id;
	uint16 startVertex;
	uint16 nVertices;
	uint16 startTriangle;
	uint16 nTriangles;
	uint16 nBones;
	uint16 StartBone;
	uint16 unknown;
	uint16 RootBone;
	Vec3D CenterMass;
	Vec3D CenterBoundingBox;
	float radius;
};

struct TextureUnit
{
	uint16 flags;
	uint16 shading;
	uint16 submeshIndex;
	uint16 submeshIndex2;
	int16 colorIndex;
	uint16 renderFlag;
	uint16 texUnitNumber;
	uint16 mode;
	uint16 texture;
	uint16 texUnitNumber2;
	uint16 transparency;
	uint16 textureAnim;
};

struct skin
{
	uint32 LOD;
	std::vector<uint16> Indices;
	std::vector<triangle> Triangles;
	std::vector<vertexProperty> Properties;
	std::vector<submesh> Submeshes;
	std::vector<TextureUnit> TextureUnits;
};