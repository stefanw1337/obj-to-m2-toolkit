#include <map>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <array>
#include <set>

#include "m2.h"
#include "m2struct.h"

/*

TODO
-> Debug AnimationBlock : seems like offset are ok now, but.. Values are fucked up ? Why are there MD20 everywhere O'o, and Goblin..Male ?!!
-> Debug Calculation of Bones size. Seems like it's wrongly calculated, hoooow is it possible \o/.

*/

#include "obj_geometry.h"
#include "collision_geometry.h"

int main(int argc, char **argv)
{
	std::cout << "M2Lib | Converter v0.1\n" << "by Garthog\n" << "Thanks to relaxok, schlumpf, gamh, mjollna, zim for code / advices they gave me.\n" << "Thanks to pxr.dk & modcraft" << std::endl;

	m2 output;
	std::string texturePath, collisionPath;
	if (argc < 3) {
		std::cerr << "Usage: OBJtoM2 <OBJFile> <OutputStem> [--texture <ArchivePath.blp>] [--collision <Trunk.obj>]\n";
		return 1;
	}
	for (int i = 3; i < argc; i += 2) {
		const std::string option = argv[i];
		if (i + 1 >= argc || (option != "--texture" && option != "--collision")) {
			std::cerr << "Invalid option or missing value: " << option << std::endl;
			return 1;
		}
		std::string& value = option == "--texture" ? texturePath : collisionPath;
		if (!value.empty() || !argv[i+1][0]) {
			std::cerr << "Duplicate option or empty value: " << option << std::endl;
			return 1;
		}
		value = argv[i+1];
	}
	obj::Mesh mesh;
	try { mesh = obj::read(argv[1]); }
	catch (const std::exception& error) {
		std::cerr << error.what() << std::endl;
		return 1;
	}

	// Meta informations

	output.setName(argv[2]);
	output.setVersion(264);

	// Dummy things

	output.AddDummyAnim();
	output.AddBone(0, 0, Vec3D());
	output.AddDummyTransparency();

	// M2 creation - Geometry

	output.AddSkin(20);

	try {
		obj::emit(mesh, output);
		if (!collisionPath.empty()) loadCollisionOBJ(collisionPath.c_str(), output);
	}
	catch (const std::exception& error) {
		std::cerr << error.what() << std::endl;
		return 1;
	}
	std::cout << "Export vertices: " << output.getVerticeCount()
	          << " triangles: " << output.getTriangleCount() << std::endl;
	if (!texturePath.empty()) {
		// Explicit single-texture mode for static props. No MTL/PBR inference.
		output.AddTexture(texturePath, 0);
		output.AddRenderFlag(0, 0);
		for (size_t i = 0; i < output.getSkins()->Submeshes.size(); ++i)
			output.AddTextureUnit(static_cast<uint16>(i), 0, 0);
	}
	else {
		std::cout << "Command line utility\nPress \"i\" to get info about the model\nPress \"te\" to add a texture\nPress \"tu\" to add a textureUnit"
			<< "\nPress \"r\" to add a renderflag\nPress \"q\" to exit and save file" << std::endl;
		std::string command;

		while(true)
		{
			if (!(std::cin >> command)) {
				std::cerr << "Input ended before q; no files saved\n";
				return 1;
			}

			if(command == "i")
			{
				std::cout << "There are " << output.getSkins()->Submeshes.size() << " submeshes" << std::endl;

				for(int i = 0; i < output.getSkins()->Submeshes.size(); i++)
				{
					std::cout << "Submesh " << i << std::endl;

					for(int j = 0; j < output.getSkins()->TextureUnits.size(); j++)
					{
						TextureUnit texu = output.getSkins()->TextureUnits.at(j);
						texture* text = output.getTextures();

						if(texu.submeshIndex == i || texu.submeshIndex2 == i)
						{
							std::cout << "-> Referenced in texUnit " << j << "( Texture " << texu.texture << " [" << text[texu.texture].Filename << "] | RenderFlag " << texu.renderFlag << " )\n" << std::endl;
						}
					}
				}
			}
			else if(command == "te")
			{
				std::string texName;

				std::cout << "Enter texture name : ";
				std::cin >> texName;

				output.AddTexture(texName.c_str(), 0);
			}
			else if(command == "tu")
			{
				uint16 submesh, texture, renderflag;
				std::string reflect;

				std::cout << "Which submesh would you like to choose ? ";
				std::cin >> submesh;

				std::cout << "What texture would you like to choose ? ";
				std::cin >> texture;

				std::cout << "What RenderFlag would you like to choose ? ";
				std::cin >> renderflag;

				std::cout << "Is it a reflect ( 'y' or 'n' ) ? ";
				std::cin >> reflect;

				if(reflect == "y")
				{
					output.AddTextureUnit(submesh, renderflag, texture, true);
				}
				else
				{
					output.AddTextureUnit(submesh, renderflag, texture);
				}
			}
			else if(command == "r")
			{
				uint16 blending, flag;

				std::cout << "Flag ";
				std::cin >> flag;

				std::cout << "Blending ";
				std::cin >> blending;

				output.AddRenderFlag(flag, blending);
			}

			else if(command == "q")
			{
				break;
			}
			else
			{
				std::cout << "Unknown command \"" << command << "\"" << std::endl;
			}
		}

	}

	std::cout << "Saving file " << argv[2] << ".m2" << std::endl;

	std::stringstream filename;
	filename << argv[2] << ".m2";

	try { output.saveToFile(filename.str()); }
	catch (const std::exception& error) {
		std::cerr << error.what() << std::endl;
		return 1;
	}

	return 0;
}
