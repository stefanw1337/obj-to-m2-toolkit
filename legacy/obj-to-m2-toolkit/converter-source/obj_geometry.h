#pragma once

#include <array>
#include <cmath>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include "m2.h"

namespace obj {
using Corner = std::array<size_t, 3>;
using Face = std::array<Corner, 3>;
struct Mesh {
    std::vector<Vec3D> positions, normals;
    std::vector<Vec2D> uvs;
    std::map<size_t, std::vector<Face>> groups;
};

inline size_t index(const std::string& text, size_t count) {
    size_t consumed = 0;
    const long long value = std::stoll(text, &consumed);
    if (consumed != text.size() || value == 0)
        throw std::runtime_error("Invalid OBJ index");
    const long long resolved = value < 0 ? static_cast<long long>(count) + value : value - 1;
    if (resolved < 0 || static_cast<size_t>(resolved) >= count)
        throw std::runtime_error("OBJ index out of range");
    return static_cast<size_t>(resolved);
}

inline Mesh read(const char* path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Cannot open OBJ file");
    Mesh mesh;
    size_t group = 0, lineNumber = 0;
    std::string line, material;
    while (std::getline(input, line)) {
        ++lineNumber;
        line = line.substr(0, line.find('#'));
        std::istringstream words(line);
        std::string token;
        if (!(words >> token)) continue;
        try {
            if (token == "v" || token == "vn") {
                Vec3D value;
                if (!(words >> value.x >> value.y >> value.z) ||
                    !std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z))
                    throw std::runtime_error("Expected three finite coordinates");
                (token == "v" ? mesh.positions : mesh.normals).push_back(value);
            } else if (token == "vt") {
                Vec2D value;
                if (!(words >> value.x >> value.y) || !std::isfinite(value.x) || !std::isfinite(value.y))
                    throw std::runtime_error("Expected two finite UV coordinates");
                mesh.uvs.push_back(value);
            } else if (token == "g") {
                ++group;
            } else if (token == "usemtl") {
                std::string next;
                if (!(words >> next)) throw std::runtime_error("Missing material name");
                if (next != material) ++group;
                material = next;
            } else if (token == "f") {
                Face face;
                for (auto& corner : face) {
                    std::string value;
                    if (!(words >> value)) throw std::runtime_error("Expected a triangular face");
                    const size_t slash1 = value.find('/');
                    const size_t slash2 = slash1 == std::string::npos ? slash1 : value.find('/', slash1 + 1);
                    if (slash1 == std::string::npos || slash2 == std::string::npos ||
                        value.find('/', slash2 + 1) != std::string::npos)
                        throw std::runtime_error("Faces require position/UV/normal indices");
                    corner = {{index(value.substr(0, slash1), mesh.positions.size()),
                               index(value.substr(slash1 + 1, slash2 - slash1 - 1), mesh.uvs.size()),
                               index(value.substr(slash2 + 1), mesh.normals.size())}};
                }
                if (words >> token) throw std::runtime_error("Triangulate the OBJ before conversion");
                mesh.groups[group].push_back(face);
            }
        } catch (const std::exception& error) {
            throw std::runtime_error("OBJ line " + std::to_string(lineNumber) + ": " + error.what());
        }
    }
    if (mesh.groups.empty()) throw std::runtime_error("OBJ has no faces");
    return mesh;
}

inline void emit(const Mesh& mesh, m2& output) {
    // This writer uses 16-bit skin indices, counts and section offsets.
    // Reject oversized inputs rather than silently wrapping any of these fields.
    const size_t limit = std::numeric_limits<uint16>::max();
    size_t indexCount = 0;
    for (const auto& group : mesh.groups) {
        if (group.second.size() > (limit - indexCount) / 3)
            throw std::runtime_error("Too many triangle indices for this writer (maximum 65535)");
        indexCount += group.second.size() * 3;
    }
    for (const auto& group : mesh.groups) {
        const size_t startVertex = output.getVerticeCount();
        const size_t startTriangle = output.getTriangleCount() * 3;
        // Keep each section contiguous. Share only identical position/UV/normal
        // tuples inside that section, preserving UV seams and hard normals.
        std::map<Corner, uint16> vertices;
        for (const auto& face : group.second) {
            std::array<uint16, 3> indices;
            for (size_t i = 0; i < 3; ++i) {
                const auto& corner = face[i];
                auto found = vertices.find(corner);
                if (found == vertices.end()) {
                    const size_t next = output.getVerticeCount();
                    if (next >= limit) throw std::runtime_error("Too many split vertices for this writer");
                    Vec2D uv = mesh.uvs.at(corner[1]);
                    uv.y = 1.0f - uv.y;
                    output.AddVertice(mesh.positions.at(corner[0]), mesh.normals.at(corner[2]), uv);
                    found = vertices.emplace(corner, static_cast<uint16>(next)).first;
                }
                indices[i] = found->second;
            }
            triangle tri = {indices[0], indices[1], indices[2]};
            output.AddTriangle(tri);
        }
        output.AddSubmesh(0, static_cast<uint16>(startVertex),
                          static_cast<uint16>(vertices.size()), static_cast<uint16>(startTriangle),
                          static_cast<uint16>(group.second.size() * 3));
    }
}
} // namespace obj
