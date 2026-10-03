#pragma once
#include "obj_geometry.h"

// Collision is a separate, triangulated OBJ in exactly the same coordinate
// system as the visible OBJ. UVs and vertex normals are deliberately ignored.
inline void loadCollisionOBJ(const char* path, m2& output) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Cannot open collision OBJ");
    std::vector<Vec3D> vertices;
    std::vector<triangle> triangles;
    std::string line;
    size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        line = line.substr(0, line.find('#'));
        std::istringstream words(line);
        std::string token;
        if (!(words >> token)) continue;
        try {
            if (token == "v") {
                Vec3D v;
                if (!(words >> v.x >> v.y >> v.z) || !std::isfinite(v.x) ||
                    !std::isfinite(v.y) || !std::isfinite(v.z))
                    throw std::runtime_error("Expected three finite coordinates");
                if (vertices.size() >= std::numeric_limits<uint16>::max())
                    throw std::runtime_error("Too many collision vertices");
                vertices.push_back(v);
            } else if (token == "f") {
                uint16 indices[3];
                for (int i = 0; i < 3; ++i) {
                    std::string value;
                    if (!(words >> value)) throw std::runtime_error("Expected triangular collision face");
                    indices[i] = static_cast<uint16>(obj::index(value.substr(0, value.find('/')), vertices.size()));
                }
                if (words >> token) throw std::runtime_error("Triangulate the collision OBJ first");
                triangles.push_back({indices[0], indices[1], indices[2]});
            }
        } catch (const std::exception& error) {
            throw std::runtime_error("Collision OBJ line " + std::to_string(lineNumber) + ": " + error.what());
        }
    }
    output.SetCollisionMesh(vertices, triangles);
}
