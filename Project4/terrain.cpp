#include "terrain.h"


bool TerrainReader::readTerrainFile(const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) 
    {
        std::cerr << "파일을 열 수 없습니다: " << filePath << std::endl;
        return false;
    }

    // 1. 헤더 읽기
    TerrainHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(TerrainHeader));

    std::cout << "지형 높이 범위: " << header.MinHeight << "m ~ " << header.MaxHeight << "m" << std::endl;

    // 2. 정점 개수(Vertex Count) 읽기
    uint32_t vertexCount;
    file.read(reinterpret_cast<char*>(&vertexCount), sizeof(uint32_t));
    std::cout << "정점 개수: " << vertexCount << std::endl;

    // 3. 정점 데이터 읽기 (u, v, height)
    // 주의: 이 데이터들은 보통 Delta-encoded 되어 있어 단순 read 후 디코딩 과정이 필요합니다.
    std::vector<uint16_t> u(vertexCount), v(vertexCount), height(vertexCount);
    file.read(reinterpret_cast<char*>(u.data()), vertexCount * sizeof(uint16_t));
    file.read(reinterpret_cast<char*>(v.data()), vertexCount * sizeof(uint16_t));
    file.read(reinterpret_cast<char*>(height.data()), vertexCount * sizeof(uint16_t));

    // 4. 인덱스 데이터 (Triangles)
    uint32_t triangleCount;
    file.read(reinterpret_cast<char*>(&triangleCount), sizeof(uint32_t));
    // ... 이후 인덱스 파싱 로직
    parseIndices(file, triangleCount);

    file.close();
    return true;
}

void TerrainReader::parseIndices(std::ifstream& file, uint32_t vertexCount)
{
    // 1. 삼각형 개수 읽기
    uint32_t triangleCount;
    file.read(reinterpret_cast<char*>(&triangleCount), sizeof(uint32_t));

    // 인덱스 데이터는 총 triangleCount * 3 개입니다.
    uint32_t totalIndices = triangleCount * 3;
    std::vector<uint16_t> encodedIndices(totalIndices);
    file.read(reinterpret_cast<char*>(encodedIndices.data()), totalIndices * sizeof(uint16_t));

    // 2. High-Water Mark 디코딩
    std::vector<uint16_t> decodedIndices(totalIndices);
    int highest = 0;
    for (size_t i = 0; i < totalIndices; ++i) 
    {
        uint16_t code = encodedIndices[i];
        decodedIndices[i] = highest - code;
        if (code == 0) 
        {
            highest++;
        }
    }

    std::cout << "삼각형 개수: " << triangleCount << std::endl;
    if (!decodedIndices.empty()) 
    {
        std::cout << "첫 번째 삼각형 인덱스: "
            << decodedIndices[0] << ", "
            << decodedIndices[1] << ", "
            << decodedIndices[2] << std::endl;
    }

    // 3. (옵션) 에지 인덱스 읽기 (West, South, East, North)
    // 지형 타일 간의 이음새를 처리하기 위한 데이터입니다.
    parseEdgeIndices(file);
}


void TerrainReader::parseEdgeIndices(std::ifstream& file)
{
    auto readEdge = [&](const std::string& name) 
    {
        uint32_t count;
        file.read(reinterpret_cast<char*>(&count), sizeof(uint32_t));
        std::vector<uint16_t> indices(count);
        file.read(reinterpret_cast<char*>(indices.data()), count * sizeof(uint16_t));
        std::cout << name << " 에지 인덱스 개수: " << count << std::endl;
    };

    readEdge("West");
    readEdge("South");
    readEdge("East");
    readEdge("North");
}

int main() {
    TerrainReader reader;
    reader.readTerrainFile("D:/mapdata/terrain/2/0/0.terrain");
    return 0;
}