#include <iostream>
#include <string>
#include <cmath>
#include <filesystem>
#include <curl/curl.h>


#define     M_PI    3.14195

namespace fs = std::filesystem;

// Web Mercator(EPSG:3857) 상수
const double MAX_EXTENT = 20037508.3427892;
const double size = MAX_EXTENT * 2.0;

// 타일 좌표(z, x, y)를 EPSG:3857 BBOX로 변환
//void getBBox(int z, int x, int y, double& minX, double& minY, double& maxX, double& maxY) 
//{
//    double numTiles = std::pow(2, z);
//    double tileSize = size / numTiles;
//
//    minX = -MAX_EXTENT + (x * tileSize);
//    maxX = -MAX_EXTENT + ((x + 1) * tileSize);
//    // Y축은 위아래가 반대인 경우가 많음 (TMS vs WMTS)
//    maxY = MAX_EXTENT - (y * tileSize);
//    minY = MAX_EXTENT - ((y + 1) * tileSize);
//}

void getBBox(int z, int x, int y, double& minLon, double& minLat, double& maxLon, double& maxLat) 
{
    double n = std::pow(2.0, z);

    // 경도 계산 (X)
    minLon = x / n * 360.0 - 180.0;
    maxLon = (x + 1) / n * 360.0 - 180.0;

    // 위도 계산 (Y) - 위도는 위아래가 반대이며 로그 투영이 들어감
    auto tile2lat = [](int y, double n) 
    {
        double r2d = 180.0 / M_PI;
        double lat_rad = atan(sinh(M_PI * (1 - 2 * y / n)));
        return lat_rad * r2d;
    };

    maxLat = tile2lat(y, n);      // 타일의 상단
    minLat = tile2lat(y + 1, n);  // 타일의 하단
}


size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) 
{
    return fwrite(ptr, size, nmemb, stream);
}

// 단일 타일 다운로드 함수
bool downloadTile(CURL* curl, int z, int x, int y, const std::string& mapPath) 
{
    double minX, minY, maxX, maxY;
    getBBox(z, x, y, minX, minY, maxX, maxY);

    // 경로 생성 (tiles/z/x/y.png)
    std::string dirPath = "tiles/" + std::to_string(z) + "/" + std::to_string(x);
    fs::create_directories(dirPath);
    std::string filePath = dirPath + "/" + std::to_string(y) + ".png";

    // WMS URL 생성
    char url[1024];
    sprintf(url, "%s?map=%s&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
        "&LAYERS=height&MODE=map&CRS=EPSG:3857&WIDTH=256&HEIGHT=256"
        "&FORMAT=image/png&STYLES=&BBOX=%f,%f,%f,%f",
        "http://10.240.33.120/cgi-bin/mapserv.exe", mapPath.c_str(), minX, minY, maxX, maxY);

    FILE* fp = fopen(filePath.c_str(), "wb");
    if (!fp) return false;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    CURLcode res = curl_easy_perform(curl);
    fclose(fp);

    return (res == CURLE_OK);
}

int main()
{
    std::string mapFile = "/ms4w/apps/local-demo/height.map";
    CURL* curl = curl_easy_init();

    if (curl) 
    {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

        // 예시: 줌 레벨 2에서 모든 타일(0~3, 0~3) 생성
        int zoom = 2;
        int maxTile = std::pow(2, zoom);

        for (int x = 0; x < maxTile; ++x) 
        {
            for (int y = 0; y < maxTile; ++y)
            {
                std::cout << "저장 중: Z=" << zoom << ", X=" << x << ", Y=" << y << "..." << std::endl;
                if (!downloadTile(curl, zoom, x, y, mapFile))
                {
                    std::cerr << "실패!" << std::endl;
                }
            }
        }
        curl_easy_cleanup(curl);
    }
    return 0;
}
