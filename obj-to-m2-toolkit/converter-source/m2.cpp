#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <stdexcept>

#include "m2.h"

/*
template< typename T>
typename T::size_type num_unique_keys( const T& mmap )
{
    if( mmap.size() < 2U ) return mmap.size() ;

    typename T::size_type n = 0 ;

    for( auto iter = mmap.begin() ; iter != mmap.end() ; iter = mmap.upper_bound( iter->first ) )
		++n;

    return n ;
}*/

std::string removeExtension(const std::string &filename) {
    size_t lastdot = filename.find_last_of(".");
    if (lastdot == std::string::npos) return filename;
    return filename.substr(0, lastdot); 
}

m2::m2(void)
{

}


m2::~m2(void)
{

}

bool m2::loadFromFile(std::string path)
{
	std::ifstream m2file;
	m2header header;

	m2file.open( path, std::ios::binary );

	m2file.seekg( 0 );
	m2file.read( reinterpret_cast<char*>(&header), sizeof(m2header) );
	
	// Header parsing;
	m_version = header.version;

	m2file.seekg( header.nameOfs );
	ReadString(m_name, m2file,header.nameOfs, header.nameLength);

	loadAnimations(m2file, header.ofsAnimations, header.nAnimations);
	loadBones(m2file, header.ofsBones, header.nBones);
	loadVertices(m2file, header.ofsVertices, header.nVertices);
	loadTextures(m2file, header.ofsTextures, header.nTextures);
	loadRenderFlags(m2file, header.ofsRenderFlags, header.nRenderFlags);

	loadLookupTable(m2file, header.ofsAttachLookup, header.nAttachLookup, AttachLookupTable);
	loadLookupTable(m2file, header.ofsAnimationLookup, header.nAnimationLookup, AnimationLookupTable);
	loadLookupTable(m2file, header.ofsBoneLookupTable, header.nBoneLookupTable, BoneLookupTable);
	loadLookupTable(m2file, header.ofsCameraLookup, header.nCameraLookup, CameraLookupTable);
	loadLookupTable(m2file, header.ofsKeyBoneLookup, header.nKeyBoneLookup, KeyBoneLookupTable);
	loadLookupTable(m2file, header.ofsTexAnimLookup, header.nTexAnimLookup, UVAnimLookupTable);
	loadLookupTable(m2file, header.ofsTexLookup, header.nTexLookup, TextureLookupTable);
	loadLookupTable(m2file, header.ofsTexUnitLookup, header.nTexUnitLookup, TextureUnitLookupTable);
	loadLookupTable(m2file, header.ofsTexReplace, header.nTexReplace, ReplaceableTextureLookupTable);
	loadLookupTable(m2file, header.ofsTransparencyLookup, header.nTransparencyLookup, TransLookupTable);

	loadViews(path, header.nViews);

	std::cout << "Loaded model " << m_name << " v" << m_version << std::endl;
	std::cout << Animations.size() << " animations\n";
	std::cout << Bones.size() << " bones\n";
	std::cout << Vertices.size() << " vertices\n";
	std::cout << Textures.size() << " textures\n";

	return true;
}

void m2::saveToFile(std::string path)
{
	std::cout << "Saving file" << std::endl;
	std::ofstream output;
	output.exceptions(std::ios::failbit | std::ios::badbit);
	output.open(path, std::ios::binary);
	
	// first the model header
	m2header header;
	std::cout << "Calculating datas" << std::endl;

	UpdateCollision();
	calcBoundingBox();
	calcVertexBox();
	if (HasDummyAnimation) {
		for (auto& anim : Animations) {
			anim.MinimumExtent = VertexBox[1];
			anim.MaximumExtent = VertexBox[0];
			anim.BoundRadius = VertexRadius;
		}
	}

	std::cout << "Filling header" << std::endl;
	fillM2Header(header);

	output.write((char*)(&header), sizeof(m2header));
	output.write(m_name.c_str(), header.nameLength);

	std::cout << "Writing animations" << std::endl;
	writeAnimations(output);
	writeLookupTable(output, AnimationLookupTable);
	std::cout << "Writing bones" << std::endl;
	writeBones(output);
	writeBonesAblock(output);
	writeLookupTable(output, KeyBoneLookupTable);
	std::cout << "Writing vertices" << std::endl;
	writeVertices(output);
	std::cout << "Writing textures" << std::endl;
	writeTextures(output);
	std::cout << "Writing transparency" << std::endl;
	writeTransparency(output);
	writeLookupTable(output, ReplaceableTextureLookupTable);
	std::cout << "Writing renderflag" << std::endl;
	writeRenderFlag(output);
	writeLookupTable(output, BoneLookupTable);
	writeLookupTable(output, TextureLookupTable);
	writeLookupTable(output, TextureUnitLookupTable);
	writeLookupTable(output, TransLookupTable);
	writeLookupTable(output, UVAnimLookupTable);
	std::cout << "Writing bounding datas" << std::endl;
	writeBoundingData(output);
	writeLookupTable(output, AttachLookupTable);
	writeLookupTable(output, CameraLookupTable);

	output.close();
	writeViews(path);
}

void m2::printTextures()
{
	for(std::vector<texture>::iterator it = Textures.begin(); it != Textures.end(); it++)
	{
		std::cout << "Texture " << (*it).Filename << " ( type " << (*it).Type << " ) with flag " << (*it).Flags << std::endl;
	}
}

void m2::printSize()
{
	uint32 totalsize = 0;
	totalsize += calcHeaderSize();
	totalsize += m_name.length();
	totalsize += calcAnimationsSize();
	totalsize += calcLookupTableSize(AnimationLookupTable);
	totalsize += calcBonesSize(true, true);

	std::cout << "Estimated size of " << totalsize << " bytes ( bonesize == " << calcBonesSize(true, true) << " )" << std::endl;
}

// Saving internal

void m2::fillM2Header(m2header &header)
{
	uint32 position = 0;

	header.id[0] = 'M';
	header.id[1] = 'D';
	header.id[2] = '2';
	header.id[3] = '0';

	header.version = m_version;
	header.nameLength = m_name.length();
	position += calcHeaderSize();

	header.nameOfs = position;
	position += m_name.length();
	
	header.Flags = 0;
	header.nGlobalSequences = 0;
	header.ofsGlobalSequences = 0;
	//position += calcGlobalSequencesSize();

	header.nAnimations = Animations.size();
	header.ofsAnimations = position;
	position += calcAnimationsSize();

	header.nAnimationLookup = AnimationLookupTable.size();
	header.ofsAnimationLookup = position;
	position += calcLookupTableSize(AnimationLookupTable);

	header.nBones = Bones.size();
	header.ofsBones = position;
	position += calcBonesSize(true, true); // change to true, true when animationsblock writing supports values

	header.nKeyBoneLookup = KeyBoneLookupTable.size();
	header.ofsKeyBoneLookup = position;
	position += calcLookupTableSize(KeyBoneLookupTable);

	header.nVertices = Vertices.size();
	header.ofsVertices = position;
	position += calcVerticesSize();

	header.nViews = Views.size();
	header.nColors = 0;
	header.ofsColors = 0;

	header.nTextures = Textures.size();
	header.ofsTextures = position;
	position += calcTexturesSize();

	header.nTransparency = Transparency.size();
	header.ofsTransparency = position;
	position += calcTransparencySize();

	header.nTexAnims = 0;
	header.ofsTexAnims = 0;
	header.nTexReplace = ReplaceableTextureLookupTable.size();
	header.ofsTexReplace = position;
	position += calcLookupTableSize(ReplaceableTextureLookupTable);

	header.nRenderFlags = RenderFlags.size();
	header.ofsRenderFlags = position;
	position += RenderFlags.size() * sizeof(renderFlag); // Should do the thing \o/

	header.nBoneLookupTable = BoneLookupTable.size();
	header.ofsBoneLookupTable = position;
	position += calcLookupTableSize(BoneLookupTable);

	header.nTexLookup = TextureLookupTable.size();
	header.ofsTexLookup = position;
	position += calcLookupTableSize(TextureLookupTable);

	header.nTexUnitLookup = TextureUnitLookupTable.size();
	header.ofsTexUnitLookup = position;
	position += calcLookupTableSize(TextureUnitLookupTable);

	header.nTransparencyLookup = TransLookupTable.size();
	header.ofsTransparencyLookup = position;
	position += calcLookupTableSize(TransLookupTable);

	header.nTexAnimLookup = UVAnimLookupTable.size();
	header.ofsTexAnimLookup = position;
	position += calcLookupTableSize(UVAnimLookupTable);

	// Floats
	header.VertexBoxMax = VertexBox[0];
	header.VertexBoxMin = VertexBox[1];
	header.VertexBoxRadius = VertexRadius;

	header.BoundingBoxMax = BoundingBox[0];
	header.BoundingBoxMin = BoundingBox[1];
	header.BoundingBoxRadius = BoundingRadius;

	header.nBoundingTriangles = BoundingTriangles.size() * 3;
	header.ofsBoundingTriangles = position;
	position += BoundingTriangles.size() * sizeof(triangle);

	header.nBoundingVertices = BoundingVertices.size();
	header.ofsBoundingVertices = position;
	position += BoundingVertices.size() * sizeof(Vec3D);

	header.nBoundingNormals = BoundingNormals.size();
	header.ofsBoundingNormals = position;
	position += BoundingNormals.size() * sizeof(Vec3D);

	header.nAttachments = 0;
	header.ofsAttachments = 0;
	header.nAttachLookup = AttachLookupTable.size();
	header.ofsAttachLookup = position;
	position += calcLookupTableSize(AttachLookupTable);

	header.nAttachments_2 = 0;
	header.ofsAttachments_2 = 0;
	header.nLights = 0;
	header.ofsLights = 0;
	header.nCameras = 0;
	header.ofsCameras = 0;
	header.nCameraLookup = CameraLookupTable.size();
	header.ofsCameraLookup = position;
	position += calcLookupTableSize(CameraLookupTable);

	header.nRibbonEmitters = 0;
	header.ofsRibbonEmitters = 0;
	header.nParticleEmitters = 0;
	header.ofsParticleEmitters = 0;
}

uint32 m2::calcHeaderSize()
{
	// todo implement flags
	return 0x130;
}

uint32 m2::calcAnimationsSize()
{
	return Animations.size() * sizeof(animation);
}

uint32 m2::calcBonesSize(bool withAnimationBlock,  bool withAnimationBlockValues)
{
	uint32 totalsize = 0;

	totalsize += Bones.size() * sizeof(m2bone);

	if(withAnimationBlock)
	{
		for(std::vector<bone>::iterator it = Bones.begin(); it != Bones.end(); it++)
		{
			uint32 TranslationSize, RotationSize, ScaleSize;

			TranslationSize = calcAnimationBlockSize<Vec3D >((*it).Translation, withAnimationBlockValues);
			RotationSize = calcAnimationBlockSize<Short4Vector >((*it).Rotation, withAnimationBlockValues);
			ScaleSize = calcAnimationBlockSize<Vec3D >((*it).Scaling, withAnimationBlockValues);

			totalsize += TranslationSize;
			totalsize += RotationSize;
			totalsize += ScaleSize;
		}
	}

	return totalsize;
}

uint32 m2::calcVerticesSize()
{
	return Vertices.size() * sizeof(vertice);
}

uint32 m2::calcTransparencySize()
{
	uint32 totalsize = 0;

	totalsize += Transparency.size() * sizeof(m2ABlock);

	for(std::vector<transparency>::iterator it = Transparency.begin(); it != Transparency.end(); it++)
	{
		totalsize += calcAnimationBlockSize<short>(it->Alpha, true);
	}

	return totalsize;
}

uint32 m2::calcTexturesSize(bool withFilename)
{
	uint32 totalsize = 0;
	
	totalsize += (Textures.size() * sizeof(m2texture));
	
	if(withFilename)
	{
		for(std::vector<texture>::iterator it = Textures.begin(); it != Textures.end(); it++)
		{
			totalsize += (*it).Filename.length();
			totalsize += sizeof(uint32);
		}
	}

	return totalsize;
}


void m2::writeAnimations(std::ofstream &stream)
{
	for(std::vector<animation>::iterator it = Animations.begin(); it != Animations.end(); it++)
	{
		stream.write(reinterpret_cast<char*>(&(*it)), sizeof(animation));
	}
}

void m2::writeBones(std::ofstream &stream)
{
	uint32 totalABlockLenght = 0;
	uint32 baseABlockOffset = (uint32)stream.tellp()  + calcBonesSize(false, false); 
	// should move that somewhere else

	//std::cout << "m2::writeBones SubAnimationBlock BASE OFFSET " << std::hex << baseABlockOffset << std::endl;

	for(std::vector<bone>::iterator it = Bones.begin(); it != Bones.end(); it++)
	{
		m2bone b;
		
		uint32 TranslationSize = 0, RotationSize = 0, ScalingSize = 0;

		b.Flags = (*it).Flags;
		b.KeyBoneId = (*it).KeyBoneId;
		b.ParentBone = (*it).ParentBone;
		b.PivotPoint = (*it).PivotPoint;
		b.Unknown[0] = (*it).Unknown[0];
		b.Unknown[1] = (*it).Unknown[1];
		b.Unknown[2] = (*it).Unknown[2];
			
		b.Translation.InterpolationType = (*it).TranslationInterp;
		b.Translation.GlobalSequenceID = (*it).TranslationGlobalSequenceID;

		b.Rotation.InterpolationType = (*it).RotationInterp;
		b.Rotation.GlobalSequenceID = (*it).RotationGlobalSequenceID;

		b.Scaling.InterpolationType = (*it).ScalingInterp;
		b.Scaling.GlobalSequenceID = (*it).ScalingGlobalSequenceID;

		if((*it).Translation.size() > 0)
		{
			TranslationSize = calcAnimationBlockSize((*it).Translation, false);

			b.Translation.numberOfTimestampPairs = (*it).Translation.size();
			b.Translation.offsetToTimestampPairs = totalABlockLenght + baseABlockOffset;
			b.Translation.numberOfKeyFramePairs = (*it).Translation.size();
			b.Translation.offsetToKeyFramePairs = totalABlockLenght + baseABlockOffset + (*it).Translation.size() * sizeof(m2SubABlock);
		}
		else
		{
			b.Translation.numberOfTimestampPairs = 0;
			b.Translation.offsetToTimestampPairs = 0;
			b.Translation.numberOfKeyFramePairs = 0;
			b.Translation.offsetToKeyFramePairs = 0;
		}

		if((*it).Rotation.size() > 0)
		{
			RotationSize = calcAnimationBlockSize((*it).Rotation, false);
			b.Rotation.numberOfTimestampPairs = (*it).Rotation.size();
			b.Rotation.offsetToTimestampPairs = totalABlockLenght + baseABlockOffset + TranslationSize;
			b.Rotation.numberOfKeyFramePairs = (*it).Rotation.size();
			b.Rotation.offsetToKeyFramePairs = totalABlockLenght + baseABlockOffset + TranslationSize + (*it).Rotation.size() * sizeof(m2SubABlock);
		}
		else
		{
			b.Rotation.numberOfTimestampPairs = 0;
			b.Rotation.offsetToTimestampPairs = 0;
			b.Rotation.numberOfKeyFramePairs = 0;
			b.Rotation.offsetToKeyFramePairs = 0;
		}

		if((*it).Scaling.size() > 0)
		{
			ScalingSize = calcAnimationBlockSize((*it).Scaling, false);
			b.Scaling.numberOfTimestampPairs = (*it).Scaling.size();
			b.Scaling.offsetToTimestampPairs = totalABlockLenght + baseABlockOffset + TranslationSize + RotationSize;
			b.Scaling.numberOfKeyFramePairs = (*it).Scaling.size();
			b.Scaling.offsetToKeyFramePairs = totalABlockLenght + baseABlockOffset + TranslationSize + RotationSize + (*it).Scaling.size() * sizeof(m2SubABlock);
		}
		else
		{
			b.Scaling.numberOfTimestampPairs = 0;
			b.Scaling.offsetToTimestampPairs = 0;
			b.Scaling.numberOfKeyFramePairs = 0;
			b.Scaling.offsetToKeyFramePairs = 0;
		}

		totalABlockLenght += RotationSize + TranslationSize + ScalingSize;

		stream.write(reinterpret_cast<char*>(&b), sizeof(m2bone));	
	}
}

void m2::writeBonesAblock(std::ofstream &stream)
{
	uint32 SubABlockBaseOffset = stream.tellp();
	uint32 WrittenLength = 0;
	uint32 SubABlockSize = 0, TranslationSize = 0, RotationSize = 0, ScalingSize = 0;
	
	// First we need size of the substructure, so we can know what offset the substructure should point to :x
	for(std::vector<bone>::iterator it = Bones.begin(); it != Bones.end(); it++)
	{
		if((*it).Translation.size() > 0)
		{
			TranslationSize = calcAnimationBlockSize((*it).Translation, false);
			SubABlockSize += TranslationSize;
		}

		if((*it).Rotation.size() > 0)
		{
			RotationSize = calcAnimationBlockSize((*it).Rotation, false);
			SubABlockSize += RotationSize;
		}
		
		if((*it).Scaling.size() > 0)
		{
			ScalingSize = calcAnimationBlockSize((*it).Scaling, false);
			SubABlockSize += ScalingSize;
		}
	}

	//std::cout << "m2::writeBonesAblock SubAnimationBlock BASE OFFSET " << std::hex << SubABlockBaseOffset << std::endl;
	//std::cout << "m2::writeBonesAblock SubAnimationBlock VALUES BASE OFFSET " << std::hex << SubABlockBaseOffset + SubABlockSize << std::endl;

	for(std::vector<bone>::iterator it = Bones.begin(); it != Bones.end(); it++)
	{
		// I need to iterate through all unique key
		if((*it).Translation.size() > 0)
		{
			for(int i = 0; i < (*it).Translation.size(); i++)
			{	
				m2SubABlock TimestampPairs;
				TimestampPairs.nValues = (*it).Translation.at(i).size();

				if((*it).Translation.at(i).size() > 0)
				{
					TimestampPairs.ofsValues = SubABlockBaseOffset + SubABlockSize + WrittenLength; // This is a defi.
					WrittenLength += sizeof(uint32) * (*it).Translation.at(i).size();
				}
				else
				{
					TimestampPairs.ofsValues = 0;
				}
				
				stream.write(reinterpret_cast<char*>(&TimestampPairs), sizeof(m2SubABlock));
			}

			for(int i = 0; i < (*it).Translation.size(); i++)
			{	
				m2SubABlock KeyFramePairs;
				KeyFramePairs.nValues = (*it).Translation.at(i).size();

				if((*it).Translation.at(i).size() > 0)
				{
					KeyFramePairs.ofsValues = SubABlockBaseOffset + SubABlockSize + WrittenLength; // This is a defi.
					WrittenLength += sizeof(Vec3D) * (*it).Translation.at(i).size();
				}
				else
				{
					KeyFramePairs.ofsValues = 0;
				}
				
				stream.write(reinterpret_cast<char*>(&KeyFramePairs), sizeof(m2SubABlock));
			}
		}

		if((*it).Rotation.size() > 0)
		{
			for(int i = 0; i < (*it).Rotation.size(); i++)
			{
				m2SubABlock TimestampPairs;
				TimestampPairs.nValues = (*it).Rotation.at(i).size();

				if((*it).Rotation.at(i).size() > 0)
				{
					TimestampPairs.ofsValues = SubABlockBaseOffset + SubABlockSize + WrittenLength; // This is a defi.
					WrittenLength += sizeof(uint32) * (*it).Rotation.at(i).size();
				}
				else
				{
					TimestampPairs.ofsValues = 0;
				}

				stream.write(reinterpret_cast<char*>(&TimestampPairs), sizeof(m2SubABlock));
			}

			for(int i = 0; i < (*it).Rotation.size(); i++)
			{
				m2SubABlock KeyFramePairs;
				KeyFramePairs.nValues = (*it).Rotation.at(i).size();

				if((*it).Rotation.at(i).size() > 0)
				{
					KeyFramePairs.ofsValues = SubABlockBaseOffset + SubABlockSize + WrittenLength; // This is a defi.
					WrittenLength += sizeof(Short4Vector) * (*it).Rotation.at(i).size();
				}
				else
				{
					KeyFramePairs.ofsValues = 0;
				}

				stream.write(reinterpret_cast<char*>(&KeyFramePairs), sizeof(m2SubABlock));
			}
		}
		
		if((*it).Scaling.size() > 0)
		{
			for(int i = 0; i < (*it).Scaling.size() ; i++)
			{
				m2SubABlock TimestampPairs;
				TimestampPairs.nValues = (*it).Scaling.at(i).size();

				if((*it).Scaling.at(i).size() > 0)
				{
					TimestampPairs.ofsValues = SubABlockBaseOffset + SubABlockSize + WrittenLength; // This is a defi.
					WrittenLength += sizeof(uint32) * (*it).Scaling.at(i).size();
				}
				else
				{
					TimestampPairs.ofsValues = 0;
				}

				stream.write(reinterpret_cast<char*>(&TimestampPairs), sizeof(m2SubABlock));
			}

			for(int i = 0; i < (*it).Scaling.size() ; i++)
			{
				m2SubABlock KeyFramePairs;
				KeyFramePairs.nValues = (*it).Scaling.at(i).size();

				if((*it).Scaling.at(i).size() > 0)
				{
					KeyFramePairs.ofsValues = SubABlockBaseOffset + SubABlockSize + WrittenLength; // This is a defi.
					WrittenLength += sizeof(Vec3D) * (*it).Scaling.at(i).size();
				}
				else
				{
					KeyFramePairs.ofsValues = 0;
				}

				stream.write(reinterpret_cast<char*>(&KeyFramePairs), sizeof(m2SubABlock));
			}
		}
	}

	// BONES SUBABLOCK WRITTEN
	//std::cout << "m2::writeBonesAblock SUBABLOCK written, position " << std::hex << stream.tellp() << std::endl;

	uint32 debugvar = 0;
	for(std::vector<bone>::iterator it = Bones.begin(); it != Bones.end(); it++)
	{
		// TRANSLATION TIMESTAMPS
		for(int i = 0; i < (*it).Translation.size() ; i++)
		{
			for(int j = 0; j < (*it).Translation.at(i).size(); j++)
			{
				uint32 towrite = (*it).Translation.at(i).at(j).first;
				stream.write(reinterpret_cast<char*>(&towrite), sizeof(uint32));
			}				
		}

		// TRANSLATION VALUES

		for(int i = 0; i < (*it).Translation.size() ; i++)
		{
			for(int j = 0; j < (*it).Translation.at(i).size(); j++)
			{
				Vec3D towrite = (*it).Translation.at(i).at(j).second;
				stream.write(reinterpret_cast<char*>(&towrite), sizeof(Vec3D));
			}
		}

		//ROTATION TIMESTAMPS
		for(int i = 0; i < (*it).Rotation.size() ; i++)
		{
			for(int j = 0; j < (*it).Rotation.at(i).size(); j++)
			{
				stream.write(reinterpret_cast<char*>(&((*it).Rotation[i][j].first)), sizeof(uint32));
			}
		}

		//ROTATION VALUES
		for(int i = 0; i < (*it).Rotation.size() ; i++)
		{
			for(int j = 0; j < (*it).Rotation.at(i).size(); j++)
			{
					stream.write(reinterpret_cast<char*>(&((*it).Rotation[i][j].second)), sizeof(Short4Vector));
			}
		}
		
		// SCALING TIMESTAMPS
		for(int i = 0; i < (*it).Scaling.size() ; i++)
		{
			for(int j = 0; j < (*it).Scaling.at(i).size(); j++)
			{
				stream.write(reinterpret_cast<char*>(&((*it).Scaling[i][j].first)), sizeof(uint32));
			}
		}

		// SCALING VALUES
		for(int i = 0; i < (*it).Scaling.size() ; i++)
		{
			for(int j = 0; j < (*it).Scaling.at(i).size(); j++)
			{
				stream.write(reinterpret_cast<char*>(&((*it).Scaling[i][j].second)), sizeof(Vec3D));
			}
		}
			

	}
	// VALUES WRITTEN
	//std::cout << "m2::writeBonesAblock VALUES written, position " << std::hex << stream.tellp() << std::endl;
}

void m2::writeTransparency(std::ofstream &stream)
{
	uint32 baseABlockOffset = (uint32)stream.tellp() + sizeof(m2ABlock) * Transparency.size();
	uint32 totalLengthSubBlock = 0, WrittenLength = 0;

	for(std::vector<transparency>::iterator it = Transparency.begin(); it != Transparency.end(); it++)
	{
		m2ABlock trans;

		trans.GlobalSequenceID = -1;
		trans.InterpolationType = 0;

		trans.numberOfTimestampPairs = it->Alpha.size();
		trans.offsetToTimestampPairs = baseABlockOffset + totalLengthSubBlock;
		totalLengthSubBlock += sizeof(m2SubABlock) * it->Alpha.size();

		trans.numberOfKeyFramePairs = it->Alpha.size();
		trans.offsetToKeyFramePairs = baseABlockOffset + totalLengthSubBlock;
		totalLengthSubBlock += sizeof(m2SubABlock) * it->Alpha.size();

		stream.write(reinterpret_cast<char*>(&trans), sizeof(m2ABlock));
	}

	std::cout << std::hex << stream.tellp() << std::endl;

	for(std::vector<transparency>::iterator it = Transparency.begin(); it != Transparency.end(); it++)
	{
		for(int i = 0; i < it->Alpha.size(); i++ )
		{
			m2SubABlock TimestampPairs;
			TimestampPairs.nValues = it->Alpha.at(i).size();

			if(it->Alpha.at(i).size() > 0)
			{
				TimestampPairs.ofsValues = baseABlockOffset + totalLengthSubBlock + WrittenLength; // This is a defi.
				WrittenLength += sizeof(uint32) * it->Alpha.at(i).size();
			}
			else
			{
				TimestampPairs.ofsValues = 0;
			}

			stream.write(reinterpret_cast<char*>(&TimestampPairs), sizeof(m2SubABlock));
		}

		for(int i = 0; i < it->Alpha.size(); i++ )
		{
			m2SubABlock KeyFramePairs;
			
			KeyFramePairs.nValues = it->Alpha.at(i).size();

			if(it->Alpha.at(i).size() > 0)
			{
				KeyFramePairs.ofsValues = baseABlockOffset + totalLengthSubBlock + WrittenLength; // This is a defi.
				WrittenLength += sizeof(short) * it->Alpha.at(i).size();
			}
			else
			{
				KeyFramePairs.ofsValues = 0;
			}

			stream.write(reinterpret_cast<char*>(&KeyFramePairs), sizeof(m2SubABlock));
		}
	}

	std::cout << std::hex << stream.tellp() << std::endl;

	for(std::vector<transparency>::iterator it = Transparency.begin(); it != Transparency.end(); it++)
	{
		for(int i = 0; i < it->Alpha.size() ; i++)
		{
			for(int j = 0; j < it->Alpha.at(i).size(); j++)
			{
				uint32 towrite = it->Alpha.at(i).at(j).first;
				stream.write(reinterpret_cast<char*>(&towrite), sizeof(uint32));
			}				
		}

		std::cout << std::hex << stream.tellp() << std::endl;

		for(int i = 0; i < it->Alpha.size() ; i++)
		{
			for(int j = 0; j < it->Alpha.at(i).size(); j++)
			{
				short towrite = it->Alpha.at(i).at(j).second;
				stream.write(reinterpret_cast<char*>(&towrite), sizeof(short));
			}
		}

		std::cout << std::hex << stream.tellp() << std::endl;
	}
}

void m2::writeVertices(std::ofstream &stream)
{
	for(int i = 0; i < Vertices.size(); i++)
	{
		stream.write(reinterpret_cast<char*>(&(Vertices.at(i))), sizeof(vertice));
	}
}

void m2::writeTextures(std::ofstream &stream)
{
	uint32 totalLength = 0;
	uint32 baseOfs = (uint32)stream.tellp() + calcTexturesSize(false);

	uint32 padding = 0;

	for(uint32 i = 0; i < Textures.size(); i++)
	{
		m2texture tex;
		tex.Flags = Textures.at(i).Flags;
		tex.Type = Textures.at(i).Type;
		tex.lenFilename = Textures.at(i).Filename.length();
		tex.ofsFilename = baseOfs + totalLength + i*sizeof(uint32); // i*sizeof(uint32) == Padding

		stream.write(reinterpret_cast<char*>(&tex), sizeof(m2texture));

		totalLength += Textures.at(i).Filename.length();
	}

	for(uint32 i = 0; i < Textures.size(); i++)
	{
		stream.write(Textures.at(i).Filename.c_str(), Textures.at(i).Filename.length());
		stream.write(reinterpret_cast<char*>(&padding), sizeof(uint32));
	}
}

void m2::writeRenderFlag(std::ofstream &stream)
{
	for(std::vector<renderFlag>::iterator it = RenderFlags.begin(); it != RenderFlags.end(); it++)
	{
		stream.write(reinterpret_cast<char*>(&(*it)), sizeof(renderFlag));
	}
}

void m2::writeLookupTable(std::ofstream &stream, std::map<uint32, int16> &lookupMap)
{
	for(std::map<uint32, int16>::iterator it = lookupMap.begin(); it != lookupMap.end(); it++)
	{
		stream.write(reinterpret_cast<char*>(&((*it).second)), sizeof(int16));
	}
}

void m2::writeBoundingData(std::ofstream &stream)
{
	for(int i = 0; i < BoundingTriangles.size(); i++)
	{
		stream.write(reinterpret_cast<char*>(&(BoundingTriangles.at(i))), sizeof(triangle));
	}

	for(int i = 0; i < BoundingVertices.size(); i++)
	{
		stream.write(reinterpret_cast<char*>(&(BoundingVertices.at(i))), sizeof(Vec3D));
	}

	for(int i = 0; i < BoundingNormals.size(); i++)
	{
		stream.write(reinterpret_cast<char*>(&(BoundingNormals.at(i))), sizeof(Vec3D));
	}
}

void m2::writeViews(std::string path)
{
	std::string name = removeExtension(path);
	std::ofstream skinFile;
	skinFile.exceptions(std::ios::failbit | std::ios::badbit);
	uint32 position = 0;

	for(int i = 0; i < Views.size(); i++)
	{
		std::stringstream currentFile;
		currentFile << name << std::setw(2) << std::setfill('0') << i << ".skin";
		skinFile.open(currentFile.str(), std::ios::binary);

		std::cout << "Writing " << currentFile.str() << std::endl;

		skin view = Views.at(i);
		skinheader header;

		position = sizeof(skinheader);

		header.id[0] = 'S';
		header.id[1] = 'K';
		header.id[2] = 'I';
		header.id[3] = 'N';

		header.LOD = view.LOD;
		
		header.nIndices = view.Indices.size();
		header.ofsIndices = position;
		position += view.Indices.size() * sizeof(uint16);


		header.nTriangles = view.Triangles.size() * 3;
		header.ofsTriangles = position;
		position += view.Triangles.size() * sizeof(triangle);

		header.nProperties = view.Properties.size();
		header.ofsProperties = position;
		position += view.Properties.size() * sizeof(vertexProperty);

		header.nSubmeshes = view.Submeshes.size();
		header.ofsSubmeshes = position;
		position += view.Submeshes.size() * sizeof(submesh);

		header.nTextureUnits = view.TextureUnits.size();
		header.ofsTextureUnits = position;

		skinFile.write(reinterpret_cast<char*>(&header), sizeof(skinheader));

		// Indice writing
		for(std::vector<uint16>::iterator it = view.Indices.begin(); it != view.Indices.end(); it++)
		{
			skinFile.write(reinterpret_cast<char*>(&(*it)), sizeof(uint16));
		}

		// Triangles writing
		for(std::vector<triangle>::iterator it = view.Triangles.begin(); it != view.Triangles.end(); it++)
		{
			skinFile.write(reinterpret_cast<char*>(&(*it)), sizeof(triangle));
		}

		// Properties writing
		for(std::vector<vertexProperty>::iterator it = view.Properties.begin(); it != view.Properties.end(); it++)
		{
			skinFile.write(reinterpret_cast<char*>(&(*it)), sizeof(vertexProperty));
		}

		// Submeshes writing
		for(std::vector<submesh>::iterator it = view.Submeshes.begin(); it != view.Submeshes.end(); it++)
		{
			skinFile.write(reinterpret_cast<char*>(&(*it)), sizeof(submesh));
		}

		// Texture unit writing
		for(std::vector<TextureUnit>::iterator it = view.TextureUnits.begin(); it != view.TextureUnits.end(); it++)
		{
			skinFile.write(reinterpret_cast<char*>(&(*it)), sizeof(TextureUnit));
		}

		// done, next.
		skinFile.close();
	}
}

template<class T>
uint32 m2::calcAnimationBlockSize(std::vector<std::vector<std::pair< uint32, T > > >  &storageMap, bool withValue)
{
	uint32 totalsize = 0;

	totalsize += storageMap.size() * (2 * sizeof(m2SubABlock)) ; // Size of two subblock with uint32 uint32

	if(withValue)
	{
		for(int i = 0; i < storageMap.size(); i++)
		{
				totalsize += storageMap.at(i).size() * sizeof(T);
				totalsize += storageMap.at(i).size() * sizeof(uint32);
		}
	}

	return totalsize;
}

// Loading internal

void m2::loadAnimations(std::ifstream &stream, uint32 offset, uint32 n)
{
	stream.seekg( offset );

	for(int i = 0; i < n; i++)
	{
		animation anim;
		stream.read(reinterpret_cast<char*>(&anim), sizeof(animation));
		Animations.push_back(anim);
	}
}

void m2::loadBones(std::ifstream &stream, uint32 offset, uint32 n)
{
	stream.seekg( offset );

	for(int i = 0; i < n; i++)
	{
		m2bone m2b;
		stream.read( reinterpret_cast<char*>(&m2b), sizeof(m2bone));

		// Convertion
		bone b;
		b.Flags = m2b.Flags;
		b.KeyBoneId = m2b.KeyBoneId;
		b.ParentBone = m2b.ParentBone;
		b.PivotPoint = m2b.PivotPoint;
		b.Unknown[0] = m2b.Unknown[0];
		b.Unknown[1] = m2b.Unknown[1];
		b.Unknown[2] = m2b.Unknown[2];

		ReadAnimBlock<Vec3D>(b.Translation, stream, m2b.Translation);
		ReadAnimBlock<Vec3D>(b.Scaling, stream, m2b.Scaling);
		ReadAnimBlock<Short4Vector>(b.Rotation, stream, m2b.Rotation);

		b.TranslationInterp = m2b.Translation.InterpolationType;
		b.TranslationGlobalSequenceID = m2b.Translation.GlobalSequenceID;

		b.RotationInterp = m2b.Rotation.InterpolationType;
		b.RotationGlobalSequenceID = m2b.Rotation.GlobalSequenceID;

		b.ScalingInterp = m2b.Scaling.InterpolationType;
		b.ScalingGlobalSequenceID = m2b.Scaling.GlobalSequenceID;

		Bones.push_back(b);
	}
}

void m2::loadVertices(std::ifstream &stream, uint32 offset, uint32 n)
{
	stream.seekg( offset );

	for(int i = 0; i < n; i++)
	{
		vertice vert;
		stream.read( reinterpret_cast<char*>(&vert), sizeof(vertice));
		Vertices.push_back(vert);
	}
}

void m2::loadTextures(std::ifstream &stream, uint32 offset, uint32 n)
{
	stream.seekg( offset );

	for(int i = 0; i < n; i++)
	{
		m2texture m2tex;
		texture tex;

		stream.read( reinterpret_cast<char*>(&m2tex), sizeof(m2texture));
		
		tex.Flags = m2tex.Flags;
		tex.Type = m2tex.Type;
		ReadString(tex.Filename, stream, m2tex.ofsFilename, m2tex.lenFilename);

		Textures.push_back(tex);
	}
}

void m2::loadRenderFlags(std::ifstream &stream, uint32 offset, uint32 n)
{
	stream.seekg( offset );

	for(int i = 0; i < n; i++)
	{
		renderFlag flag;
		stream.read( reinterpret_cast<char*>(&flag), sizeof(renderFlag));
		RenderFlags.push_back(flag);
	}
}

void m2::loadTransparency(std::ifstream &stream, uint32 offset, uint32 n)
{
	stream.seekg( offset ) ;

	for(int i = 0; i < n; i++)
	{
		m2transparency trans;
		stream.read( reinterpret_cast<char*>(&trans), sizeof(m2transparency));

		transparency _transparency;
		ReadAnimBlock<short>(_transparency.Alpha, stream, trans.Alpha);

		Transparency.push_back(_transparency);
	}
}

/*
void m2::loadColors(std::ifstream &stream, uint32 offset, uint32 n);

void m2::loadUVAnimation(std::ifstream &stream, uint32 offset, uint32 n);
void m2::loadTexReplace(std::ifstream &stream, uint32 offset, uint32 n);

void m2::loadBoundingTriangles(std::ifstream &stream, uint32 offset, uint32 n);
void m2::loadBoundingVertices(std::ifstream &stream, uint32 offset, uint32 n);
void m2::loadBoundingNormals(std::ifstream &stream, uint32 offset, uint32 n);
void m2::loadAttachments(std::ifstream &stream, uint32 offset, uint32 n);
void m2::loadEvents(std::ifstream &stream, uint32 offset, uint32 n);
void m2::loadCameras(std::ifstream &stream, uint32 offset, uint32 n);
void m2::loadRibbonEmitters(std::ifstream &stream, uint32 offset, uint32 n);
void m2::loadParticleEmitters(std::ifstream &stream, uint32 offset, uint32 n);
*/

void m2::loadLookupTable(std::ifstream &stream, uint32 offset, uint32 n, std::map<uint32, int16> &lookupMap)
{
	stream.seekg( offset );

	for(int i = 0; i < n; i++)
	{
		int16 value;
		stream.read( reinterpret_cast<char*>(&value), sizeof(int16));
		lookupMap.insert(std::make_pair(i, value));
	}
}

void m2::loadViews(std::string filename, uint32 n)
{
	std::string name = removeExtension(filename);
	std::ifstream skinFile;

	std::cout << n << std::endl;
	for(int i = 0; i < n; i++)
	{
		std::stringstream currentFile;
		skin view;
		skinheader header;

		currentFile << name << std::setw(2) << std::setfill('0') << i << ".skin";

		std::cout << "Loading " << currentFile.str() << std::endl;

		skinFile.open( currentFile.str(), std::ios::binary );
		
		skinFile.read( reinterpret_cast<char*>(&(header)), sizeof(skinheader));
		
		// Info
		view.LOD = header.LOD;

		std::cout << "Indice loading" << std::endl;
		// Indices loading
		skinFile.seekg(header.ofsIndices);

		for(int j = 0; j < header.nIndices; j++)
		{
			uint16 indice;
			skinFile.read( reinterpret_cast<char*>(&indice), sizeof(uint16));
			view.Indices.push_back(indice);
		}

		std::cout << "Triangles loading" << std::endl;
		// Triangles Loading
		skinFile.seekg(header.ofsTriangles);
		
		for(int j = 0; j < (header.nTriangles/3); j++)
		{
			triangle tri;
			skinFile.read( reinterpret_cast<char*>(&tri), sizeof(triangle));
			view.Triangles.push_back(tri);
		}

		std::cout << "Vertex loading" << std::endl;
		// Vertex Properties
		skinFile.seekg(header.ofsProperties);

		for(int j = 0; j < (header.nProperties); j++)
		{
			vertexProperty prop;
			skinFile.read( reinterpret_cast<char*>(&prop), sizeof(vertexProperty));
			view.Properties.push_back(prop);
		}

		std::cout << "Submeshes loading" << std::endl;
		// Submeshes
		skinFile.seekg(header.ofsSubmeshes);

		for(int j = 0; j < (header.nSubmeshes); j++)
		{
			submesh mesh;
			skinFile.read( reinterpret_cast<char*>(&mesh), sizeof(submesh));
			view.Submeshes.push_back(mesh);
		}		

		std::cout << "Texture loading" << std::endl;
		// Texture Unit
		skinFile.seekg(header.ofsTextureUnits);

		for(int j = 0; j < (header.nTextureUnits); j++)
		{
			TextureUnit texunit;
			skinFile.read( reinterpret_cast<char*>(&texunit), sizeof(TextureUnit));
			view.TextureUnits.push_back(texunit);
		}		

		Views.push_back(view);
		skinFile.close();
	}
}

// Internal functions
void m2::ReadString(std::string &string, std::ifstream &stream, uint32 offset, uint32 length)
{
	int jumpBack = stream.tellg();
	stream.seekg( offset ); // Jump to where the texture is

	string.resize(length);
	stream.read(&string[0], length );

	stream.seekg( jumpBack ); // jump back where we were;
}

template<class T>
void m2::ReadAnimBlock(std::vector<std::vector<std::pair< uint32, T > > > &storageMap, std::ifstream &stream, m2ABlock &animblock)
{
	int jumpback = stream.tellg();
	//std::cout << "Reading animation block ( tellg " << jumpback << " )" << std::endl;

	if(animblock.numberOfKeyFramePairs == animblock.numberOfTimestampPairs)
	{
		for(uint32 i = 0; i < animblock.numberOfTimestampPairs; i++)
		{
			std::vector<std::pair<uint32, T>> vector;
			m2SubABlock timestampSubBlock, pairsSubBlock;

			stream.seekg( animblock.offsetToTimestampPairs + (i * sizeof(m2SubABlock)));
			stream.read( reinterpret_cast<char*>(&timestampSubBlock), sizeof(m2SubABlock));

			stream.seekg( animblock.offsetToKeyFramePairs + (i * sizeof(m2SubABlock)));
			stream.read( reinterpret_cast<char*>(&pairsSubBlock), sizeof(m2SubABlock));

			if(timestampSubBlock.nValues != pairsSubBlock.nValues)
			{
				std::cout << "SubBlocks at offsets " << std::hex << animblock.offsetToTimestampPairs + (i * sizeof(m2SubABlock)) << " and " << animblock.offsetToKeyFramePairs + (i * sizeof(m2SubABlock)) 
					<< " should have same nValue. It's not the case" << std::endl;
			}

			for(uint32 j = 0; j<timestampSubBlock.nValues; j++) // We just ignore NULL SubAnimBlock
			{
				uint32 Timestamp;
				T KeyFrame;

				stream.seekg( timestampSubBlock.ofsValues + (j * sizeof(uint32)));
				stream.read( reinterpret_cast<char*>(&Timestamp), sizeof(uint32));

				stream.seekg( pairsSubBlock.ofsValues + (j * sizeof(T)));
				stream.read( reinterpret_cast<char*>(&KeyFrame), sizeof(T));

				vector.push_back(std::make_pair(Timestamp, KeyFrame));
			}
			storageMap.push_back(vector);
		}

		
	}
	else
	{
		std::cout << "Wrong animblock" << std::endl;
	}

	stream.seekg(jumpback);
}


// Modification 

void m2::AddSkin(uint32 lod)
{
	skin view;
	view.LOD = lod;

	Views.push_back(view);
}

void m2::AddVertice(Vec3D position, Vec3D normal, Vec2D uvmap)
{
	vertice vert;
	vertexProperty vertprop;

	vert.BoneIndices[0] = 0;
	vert.BoneIndices[1] = 0;
	vert.BoneIndices[2] = 0;
	vert.BoneIndices[3] = 0;

	vert.BoneWeight[0] = -1;
	vert.BoneWeight[1] = 0;
	vert.BoneWeight[2] = 0;
	vert.BoneWeight[3] = 0;

	vert.Position = position;
	vert.Normal = normal;
	vert.TextureCoords = uvmap;

	vertprop.boneIndice1 = 0;
	vertprop.boneIndice2 = 0;
	vertprop.boneIndice3 = 0;
	vertprop.boneIndice4 = 0;

	Views.at(0).Indices.push_back(Vertices.size());
	Vertices.push_back(vert);
	Views.at(0).Properties.push_back(vertprop);
}

void m2::AddTriangle(triangle tri)
{
	Views.at(0).Triangles.push_back(tri);
}

void m2::AddVertice(Vec3D position, Vec3D normal, Vec2D uvmap, int index)
{
	vertice vert;
	vertexProperty vertprop;

	vert.BoneIndices[0] = 0;
	vert.BoneIndices[1] = 0;
	vert.BoneIndices[2] = 0;
	vert.BoneIndices[3] = 0;

	vert.BoneWeight[0] = -1;
	vert.BoneWeight[1] = 0;
	vert.BoneWeight[2] = 0;
	vert.BoneWeight[3] = 0;

	vert.Position = position;

	vert.Normal = normal;
	vert.TextureCoords = uvmap;

	vertprop.boneIndice1 = 0;
	vertprop.boneIndice2 = 0;
	vertprop.boneIndice3 = 0;
	vertprop.boneIndice4 = 0;

	Vertices[index] = vert;
	Views.at(0).Properties.push_back(vertprop);
}

void m2::ReserveVertices(int n)
{
	vertice vert;

	vert.BoneIndices[0] = 0;
	vert.BoneIndices[1] = 0;
	vert.BoneIndices[2] = 0;
	vert.BoneIndices[3] = 0;

	vert.BoneWeight[0] = -1;
	vert.BoneWeight[1] = 0;
	vert.BoneWeight[2] = 0;
	vert.BoneWeight[3] = 0;

	for(int i = 0; i < n; i++)
	{
		Views.at(0).Indices.push_back(i);
		Vertices.push_back(vert);
	}
}

void m2::AddTexture(std::string name, uint32 type)
{
	texture tex;

	tex.Filename = name.c_str();
	tex.Type = type;
	tex.Flags = 0;

	UVAnimLookupTable.insert(std::make_pair(UVAnimLookupTable.size(), -1));
	TextureLookupTable.insert(std::make_pair(TextureLookupTable.size(), Textures.size()));
	Textures.push_back(tex);

	// Have to understand that shit, copied from MDX to M2, but seems strange oO'
	ReplaceableTextureLookupTable.insert(std::make_pair(0, 1));
	ReplaceableTextureLookupTable.insert(std::make_pair(1, 1));
	ReplaceableTextureLookupTable.insert(std::make_pair(2, 0));

	for(int i = 3; i < 15; i++)
	{
		ReplaceableTextureLookupTable.insert(std::make_pair(i, -1));
	}
}

void m2::AddTextureUnit(uint16 submesh, uint16 renderflag, uint16 texture, bool reflect)
{
	TextureUnit texUnit;

	texUnit.flags = 16;
	texUnit.shading = 0;
	texUnit.submeshIndex = submesh;
	texUnit.submeshIndex2 = submesh;
	texUnit.colorIndex = -1;
	texUnit.renderFlag = renderflag;
	texUnit.mode = 1;
	texUnit.texture = texture;
	texUnit.transparency = 0;
	texUnit.textureAnim = 0;

	texUnit.texUnitNumber = 0;
	texUnit.texUnitNumber2 = 0;

	if(reflect == true)
	{
		texUnit.texUnitNumber = 1;
		texUnit.texUnitNumber2 = 1;
	}

	if(reflect)
	{
		TextureUnitLookupTable.insert(std::make_pair(Views.at(0).TextureUnits.size(), -1));
	}
	else
	{
		TextureUnitLookupTable.insert(std::make_pair(Views.at(0).TextureUnits.size(), 0));
	}

	Views.at(0).TextureUnits.push_back(texUnit);
}

void m2::AddSubmesh(uint32 id, uint16 StartVertex, uint16 nVertices, uint16 StartTriangle, uint16 nTriangles)
{
	submesh mesh;

	mesh.id = id;
	mesh.startVertex = StartVertex;
	mesh.startTriangle = StartTriangle;
	mesh.nVertices = nVertices;
	mesh.nTriangles = nTriangles;
	mesh.nBones = 1;
	mesh.StartBone = 0;
	mesh.unknown = 1;
	mesh.RootBone = 0;
	// Section bounds must describe the visible geometry, not its collision.
	Vec3D low = Vertices.at(StartVertex).Position;
	Vec3D high = low;
	mesh.CenterMass = Vec3D();
	for (uint32 i = StartVertex; i < uint32(StartVertex) + nVertices; ++i) {
		const Vec3D p = Vertices.at(i).Position;
		low.x = std::min(low.x, p.x); low.y = std::min(low.y, p.y); low.z = std::min(low.z, p.z);
		high.x = std::max(high.x, p.x); high.y = std::max(high.y, p.y); high.z = std::max(high.z, p.z);
		mesh.CenterMass = mesh.CenterMass + p;
	}
	mesh.CenterMass = mesh.CenterMass * (1.0f / nVertices);
	mesh.CenterBoundingBox = (low + high) * 0.5f;
	mesh.radius = 0;
	for (uint32 i = StartVertex; i < uint32(StartVertex) + nVertices; ++i)
		mesh.radius = std::max(mesh.radius, (Vertices.at(i).Position - mesh.CenterBoundingBox).length());

	Views.at(0).Submeshes.push_back(mesh);
}

void m2::AddRenderFlag(uint16 flag, uint16 blending)
{
	renderFlag rf;
	
	rf.Flags = flag;
	rf.Blending = blending;

	RenderFlags.push_back(rf);
}

void m2::AddBone(int32 keyBoneId, uint32 flags, Vec3D pivotPoint)
{
	bone b;

	b.KeyBoneId = -1;
	b.ParentBone = -1;
	b.Flags = flags;
	b.PivotPoint = pivotPoint;

	b.Unknown[0] = 0;
	b.Unknown[1] = 304;
	b.Unknown[2] = 0;

	b.TranslationGlobalSequenceID = -1;
	b.TranslationInterp = 0;
	b.RotationGlobalSequenceID = -1;
	b.RotationInterp = 0;
	b.ScalingGlobalSequenceID = -1;
	b.ScalingInterp = 0;

	KeyBoneLookupTable.insert(std::make_pair(0, -1));
	BoneLookupTable.insert(std::make_pair(BoneLookupTable.size(), Bones.size()));
	Bones.push_back(b);
}

void m2::AddDummyAnim()
{
	HasDummyAnimation = true;
	animation dummy;

	dummy.AnimationID = 0;
	dummy.SubAnimationID = 0;
	dummy.Length = 3333;
	dummy.MovingSpeed = 0;
	dummy.Flags = 32;
	dummy.Probability = 32767;
	dummy.Unknown0 = 0;
	dummy.Unknown1 = 0;
	dummy.Unknown2 = 0;
	dummy.PlaybackSpeed = 50;
	dummy.BoundRadius = 0;
	dummy.NextAnimation = -1;
	dummy.Index = 0;

	Animations.push_back(dummy);

	AnimationLookupTable.insert(std::make_pair(0, 0));

	for(int i = 1; i < 474; i++)
	{
		AnimationLookupTable.insert(std::make_pair(i, -1));
	}
}

void m2::AddDummyTransparency()
{
	transparency dummy;

	std::vector<ShortTimeline> vec;
	ShortTimeline timeline;

	timeline.first = 0;
	timeline.second = 32767;

	vec.push_back(timeline);
	dummy.Alpha.push_back(vec);

	Transparency.push_back(dummy);
	TransLookupTable.insert(std::make_pair(0, 0));
}

// Thanks Shlumpf & Mjollna


void m2::SetCollisionMesh(const std::vector<Vec3D>& vertices, const std::vector<triangle>& triangles)
{
    if (vertices.empty() || triangles.empty())
        throw std::runtime_error("Collision mesh must have vertices and triangles");
    std::vector<Vec3D> normals;
    for (const auto& tri : triangles) {
        if (tri.indice1 >= vertices.size() || tri.indice2 >= vertices.size() || tri.indice3 >= vertices.size())
            throw std::runtime_error("Collision triangle index out of range");
        const Vec3D u = vertices[tri.indice2] - vertices[tri.indice1];
        const Vec3D v = vertices[tri.indice3] - vertices[tri.indice1];
        Vec3D normal = u % v;
        const float length = normal.length();
        if (!std::isfinite(length) || length <= 0)
            throw std::runtime_error("Degenerate collision triangle");
        normal = normal * (1.0f / length);
        normals.push_back(normal);
    }
    BoundingVertices = vertices;
    BoundingTriangles = triangles;
    BoundingNormals = normals;
    HasCustomCollision = true;
}

void m2::UpdateCollision()
{
	if (HasCustomCollision) return;
	BoundingVertices.clear();
	BoundingNormals.clear();
	BoundingTriangles.clear();

	const uint32 nTriangle = getTriangleCount();
	const uint32 nIndices = getVerticeCount();

	for(int i = 0; i < nIndices; i++)
	{
		BoundingVertices.push_back(Vertices.at(i).Position);
	}

	for(int i = 0; i < nTriangle; i++)
	{
		Vec3D normal;
		triangle tri;

		tri.indice1 = Views.at(0).Triangles.at(i).indice1;
		tri.indice2 = Views.at(0).Triangles.at(i).indice2;
		tri.indice3 = Views.at(0).Triangles.at(i).indice3;

		Vec3D U ( Vertices.at(Views.at(0).Triangles.at(i).indice2).Position - Vertices.at(Views.at(0).Triangles.at(i).indice1).Position );
		Vec3D V ( Vertices.at(Views.at(0).Triangles.at(i).indice3).Position - Vertices.at(Views.at(0).Triangles.at(i).indice1).Position );

		normal = U % V;
		normal.normalize();

		BoundingNormals.push_back(normal);
		BoundingTriangles.push_back(tri);
	}
}

void m2::FlipTex()
{
	for(std::vector<vertice>::iterator it = Vertices.begin(); it != Vertices.end(); it++)
	{
		it->TextureCoords.y = 1 - it->TextureCoords.y;
	}
}

void m2::calcVertexBox()
{
	float max=-99999;
	float may=-99999;
	float maz=-99999;
	float mix=99999;
	float miy=99999;
	float miz=99999;

	for(std::vector<vertice>::iterator it = Vertices.begin(); it != Vertices.end(); it++)
	{
		if(it->Position.x > max) max = it->Position.x;
		if(it->Position.y > may) may = it->Position.y;
		if(it->Position.z > maz) maz = it->Position.z;
		if(it->Position.x < mix) mix = it->Position.x;
		if(it->Position.y < miy) miy = it->Position.y;
		if(it->Position.z < miz) miz = it->Position.z;
	}

	VertexBox[0] = Vec3D(max, may, maz);
	VertexBox[1] = Vec3D(mix, miy, miz);

	VertexRadius = 0;
	for (const auto& vertex : Vertices) VertexRadius = std::max(VertexRadius, vertex.Position.length());
}

void m2::calcBoundingBox()
{
	float max=-99999;
	float may=-99999;
	float maz=-99999;
	float mix=99999;
	float miy=99999;
	float miz=99999;

	for(std::vector<Vec3D>::iterator it = BoundingVertices.begin(); it != BoundingVertices.end(); it++)
	{
		if(it->x > max) max = it->x;
		if(it->y > may) may = it->y;
		if(it->z > maz) maz = it->z;
		if(it->x < mix) mix = it->x;
		if(it->y < miy) miy = it->y;
		if(it->z < miz) miz = it->z;
	}

	BoundingBox[0] = Vec3D(max, may, maz);
	BoundingBox[1] = Vec3D(mix, miy, miz);

	BoundingRadius = 0;
	for (const auto& vertex : BoundingVertices) BoundingRadius = std::max(BoundingRadius, vertex.length());
}