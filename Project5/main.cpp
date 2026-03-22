#include <iostream>
#include <vector>
#include <string>
#include <curl/curl.h>
#include <zlib.h>


// 1. Quantized Mesh Header 구조체 (Cesium 표준)
struct QuantizedMeshHeader {
    double CenterX, CenterY, CenterZ;
    float MinHeight, MaxHeight;
    double BoundingSphereCenterX, BoundingSphereCenterY, BoundingSphereCenterZ, BoundingSphereRadius;
    double HorizonOcclusionPointX, HorizonOcclusionPointY, HorizonOcclusionPointZ;
};

// 서버에서 받은 데이터를 저장할 구조체
struct MemoryBuffer {
    std::vector<char> data;
};

struct BoundingBox {
    double minLat, minLon;
    double maxLat, maxLon;
};

struct TileCoord {
    int z, x, y;
};

// libcurl용 콜백 함수: 데이터를 받을 때마다 호출됨
size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp) 
{
    size_t realsize = size * nmemb;
    MemoryBuffer* mem = static_cast<MemoryBuffer*>(userp);

    // 기존 데이터 뒤에 새 데이터 추가
    const char* charContents = static_cast<const char*>(contents);
    mem->data.insert(mem->data.end(), charContents, charContents + realsize);

    return realsize;
}

class TerrainDownloader 
{
public:
    TerrainDownloader() 
    {
        curl_global_init(CURL_GLOBAL_ALL);
    }

    ~TerrainDownloader() 
    {
        curl_global_cleanup();
    }

    /**
     * @param url 터레인 타일 주소 (예: "https://assets.agi.com/stk-terrain/v1/tilesets/world/tiles/0/0/0.terrain")
     * @return 다운로드된 바이너리 데이터 벡터
     */
    std::vector<char> downloadTerrain(const std::string& url) 
    {
        CURL* curl_handle = curl_easy_init();
        MemoryBuffer chunk;

        if (curl_handle) 
        {
            // 1. URL 설정
            curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());

            // 2. 콜백 함수와 버퍼 설정
            curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
            curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&chunk);

            // 3. 헤더 설정 (Quantized-mesh 서버는 보통 이 헤더를 요구합니다)
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Accept: application/vnd.quantized-mesh,application/octet-stream;q=0.9,*/*;q=0.01");
            curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

            // 4. 리다이렉트 허용 및 타임아웃
            curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 10L); // 10초 제한

            // 5. 실제 요청 실행
            CURLcode res = curl_easy_perform(curl_handle);

            if (res != CURLE_OK) 
            {
                std::cerr << "Download failed: " << curl_easy_strerror(res) << std::endl;
            }
            else 
            {
                long response_code;
                curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
                std::cout << "HTTP Status: " << response_code << " | Downloaded: " << chunk.data.size() << " bytes" << std::endl;
            }

            // 정리
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl_handle);
        }

        return chunk.data;
    }
};


class TerrainTileManager 
{
public:
    // 1. 위경도 영역을 타일 좌표 범위로 변환
    std::vector<TileCoord> getVisibleTileCoords(BoundingBox bbox, int zoom) 
    {
        std::vector<TileCoord> coords;

        // 해당 레벨의 총 타일 개수
        double tilesX = std::pow(2, zoom + 1);
        double tilesY = std::pow(2, zoom);

        // 타일 하나당 도(degree) 단위 크기
        double lonStep = 360.0 / tilesX;
        double latStep = 180.0 / tilesY;

        // 시작 및 끝 인덱스 계산
        int startX = static_cast<int>((bbox.minLon + 180.0) / lonStep);
        int endX = static_cast<int>((bbox.maxLon + 180.0) / lonStep);
        int startY = static_cast<int>((bbox.minLat + 90.0) / latStep);
        int endY = static_cast<int>((bbox.maxLat + 90.0) / latStep);

        // 범위 내의 모든 타일 좌표 생성
        for (int x = startX; x <= endX; ++x)
        {
            for (int y = startY; y <= endY; ++y) 
            {
                coords.push_back({ zoom, x, y });
            }
        }
        return coords;
    }


    // 2. URL 생성 및 다운로드 실행 (이전 단계의 TerrainDownloader 활용)
    void fetchTerrainForArea(BoundingBox bbox, int zoom) 
    {
        auto coords = getVisibleTileCoords(bbox, zoom);
        std::cout << "필요한 타일 개수: " << coords.size() << std::endl;

        for (const auto& tile : coords)
        {
            // 서버 URL 패턴 구성 (예: Cesium World Terrain)
            std::string url = "http://10.240.33.120:9000/terrain/"
                + std::to_string(tile.z) + "/"
                + std::to_string(tile.x) + "/"
                + std::to_string(tile.y) + ".terrain";

            std::cout << "다운로드 중: " << url << std::endl;

             
            // 여기서 이전에 만든 downloader.downloadTerrain(url) 호출
            // std::vector<char> data = downloader.downloadTerrain(url);
            // if(!data.empty()) processTerrain(data);

            TerrainDownloader downloader;
            std::vector<char> binaryData = downloader.downloadTerrain(url);
            std::vector<char> decompressed;

            if (!binaryData.empty()) 
            {
                std::cout << "데이터를 성공적으로 가져왔습니다. 이제 Gzip 해제 및 파싱을 시작할 수 있습니다." << std::endl;
                // 이후 로직:
                // 1. Gzip 압축 해제 (zlib 사용)
                // 2. TerrainHeader 읽기
               
                if (decompressGzip(binaryData, decompressed))                 
                {
                    processTerrainData(decompressed);
                    
                }
            }

        }
    }

    // --- gzip 압축 해제 함수 ---
    bool decompressGzip(const std::vector<char>& compressedData, std::vector<char>& decompressedData) 
    {
        if (compressedData.empty()) return false; 

        z_stream strm = {};
        strm.next_in = (Bytef*)compressedData.data();
        strm.avail_in = (uInt)compressedData.size();

        // 16을 더하면 zlib이 gzip 헤더를 자동으로 감지합니다.
        if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK)
        {
            return false;
        }

        char outbuffer[32768];
        int ret;

        do {
            strm.next_out = (Bytef*)outbuffer;
            strm.avail_out = sizeof(outbuffer);

            ret = inflate(&strm, Z_NO_FLUSH);

            if (decompressedData.size() < strm.total_out) 
            {
                decompressedData.insert(decompressedData.end(), outbuffer, outbuffer + (sizeof(outbuffer) - strm.avail_out));
            }
        } while (ret == Z_OK);

        inflateEnd(&strm);
        return (ret == Z_STREAM_END);
    }


    // 3. 압축 해제된 데이터를 파싱하고 시뮬레이션으로 "그리기"
    void processTerrainData(const std::vector<char>& data) 
    {
        if (data.size() < sizeof(QuantizedMeshHeader)) 
        {
            std::cerr << "데이터가 헤더 크기보다 작습니다." << std::endl;
            return;
        }

        // A. 헤더 읽기
        QuantizedMeshHeader header;
        std::memcpy(&header, data.data(), sizeof(QuantizedMeshHeader));

        std::cout << "\n--- 타일 정보 ---" << std::endl;
        std::cout << "최저 고도: " << header.MinHeight << "m" << std::endl;
        std::cout << "최고 고도: " << header.MaxHeight << "m" << std::endl;
        std::cout << "중심 좌표(ECEF): (" << header.CenterX << ", " << header.CenterY << ")" << std::endl;

        // B. 정점 개수 읽기 (헤더 바로 다음 4바이트)
        size_t offset = sizeof(QuantizedMeshHeader);
        uint32_t vertexCount;
        std::memcpy(&vertexCount, data.data() + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        std::cout << "정점 개수: " << vertexCount << "개" << std::endl;

        // C. 정점 데이터(u, v, height) 추출 (단순화된 예시)
        // 실제로는 zig-zag 인코딩된 데이터를 디코딩해야 하지만, 여기서는 개념적 흐름만 보입니다.
        std::cout << "화면에 지형을 렌더링합니다... (OpenGL/DirectX 호출부)" << std::endl;

        drawToScreen(header, vertexCount);
    }

    void drawToScreen(const QuantizedMeshHeader& header, uint32_t count)
    {
        // 실제 그래픽 라이브러리(OpenGL 등) 연동 시:
        // 1. VBO(Vertex Buffer Object) 생성
        // 2. header.MinHeight와 MaxHeight를 이용해 정점의 Z축 스케일링
        // 3. glDrawElements 호출
        std::cout << ">> [Rendering] " << count << " 개의 삼각형 정점을 화면에 배치함." << std::endl;
    }

    // fetchTerrainForArea 내부에서 호출하는 방식
    void fetchAndRender(const std::string& url) 
    {
        // 1. 다운로드 (생략)
        // 2. 압축 해제
        std::vector<char> compressed; /* downloader.download(url) */
        std::vector<char> decompressed;

        /*
        if (decompressGzip(compressed, decompressed)) {
            processTerrainData(decompressed);
        }
        */
    }
};


int main() {
    TerrainTileManager manager;

    // 예: 대한민국 주변 영역 (위도 33~39, 경도 124~130)
    BoundingBox korea = { 33.0, 124.0, 39.0, 130.0 };
    int zoomLevel = 3; // 원하는 상세도

    manager.fetchTerrainForArea(korea, zoomLevel);

    return 0;
}


/*
int main() {
    TerrainDownloader downloader;

    // 예시: 전 세계 터레인 타일 0/0/0 (Zoom/X/Y)
    std::string terrainUrl = "http://10.240.33.120:9000/terrain/1/1/0.terrain";

    std::vector<char> binaryData = downloader.downloadTerrain(terrainUrl);

    if (!binaryData.empty()) {
        std::cout << "데이터를 성공적으로 가져왔습니다. 이제 Gzip 해제 및 파싱을 시작할 수 있습니다." << std::endl;
        // 이후 로직:
        // 1. Gzip 압축 해제 (zlib 사용)
        // 2. TerrainHeader 읽기
        // 3. Vertex/Index 디코딩
    }

    return 0;
}
*/

/* // 실제 적용 예시:
std::vector<char> compressed = downloader.downloadTerrain(url);
if (!compressed.empty()) {
    std::vector<char> uncompressed;
    if (decompressGzip(compressed, uncompressed)) {
        std::cout << "압축 해제 성공! 크기: " << uncompressed.size() << " bytes" << std::endl;
        // 여기서 uncompressed 데이터를 파싱 (Quantized Mesh 포맷)
    } else {
        std::cerr << "압축 해제 실패" << std::endl;
    }
}
*/

