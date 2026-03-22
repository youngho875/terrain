#pragma once

#include <iostream>
#include <vector>
#include <fstream>
#include <cstdint>


// Quantized-mesh 헤더 구조체
struct TerrainHeader
{
    double CenterX, CenterY, CenterZ;
    float MinHeight, MaxHeight;
    double BoundingSphereCenterX, BoundingSphereCenterY, BoundingSphereCenterZ, BoundingSphereRadius;
    double HorizonOcclusionPointX, HorizonOcclusionPointY, HorizonOcclusionPointZ;
};

struct Triangle {
    uint16_t v1, v2, v3;
};

class TerrainReader
{
public:
    bool readTerrainFile(const std::string& filePath);
    void parseIndices(std::ifstream& file, uint32_t vertexCount);


private:
    void parseEdgeIndices(std::ifstream& file);

};
