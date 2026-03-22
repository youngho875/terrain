#include <iostream>
#include <string>
#include <cmath>
#include <filesystem>
#include <curl/curl.h>


#define     M_PI    3.14159265358979323846


namespace fs = std::filesystem;

// 1. 위경도를 타일 번호(X, Y)로 변환
void latLonToTile(double lat, double lon, int zoom, int& x, int& y) 
{
    double n = std::pow(2.0, zoom);
    x = static_cast<int>((lon + 180.0) / 360.0 * n);
    double lat_rad = lat * M_PI / 180.0;
    y = static_cast<int>((1.0 - std::log(std::tan(lat_rad) + (1.0 / std::cos(lat_rad))) / M_PI) / 2.0 * n);
}

// 2. 타일 번호를 WMS BBOX 위경도로 변환
void tileToBBox(int z, int x, int y, double& minLon, double& minLat, double& maxLon, double& maxLat) 
{
    double n = std::pow(2.0, z);
    auto tile2lon = [n](int x) 
    { return x / n * 360.0 - 180.0;
    };
    auto tile2lat = [n](int y) 
    {
        double lat_rad = atan(sinh(M_PI * (1 - 2 * y / n)));
        return lat_rad * 180.0 / M_PI;
    };
    minLon = tile2lon(x);
    maxLon = tile2lon(x + 1);
    maxLat = tile2lat(y); 
    minLat = tile2lat(y + 1);
}

// 3. 파일 저장 콜백
size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) 
{
    return fwrite(ptr, size, nmemb, stream);
}

int main() 
{
    // 설정 정보
    std::string mapFile = "/ms4w/apps/local-demo/height.map";
    std::string baseUrl = "http://10.240.33.120/cgi-bin/mapserv.exe";
    int zoom = 10;

    // 영역 설정 (예: 제주도 인근)
    double areaMinLon = 126.1, areaMaxLon = 126.9;
    double areaMinLat = 33.2, areaMaxLat = 33.6;

    // 범위 계산
    int startX, startY, endX, endY;
    latLonToTile(areaMaxLat, areaMinLon, zoom, startX, startY);
    latLonToTile(areaMinLat, areaMaxLon, zoom, endX, endY);

    // libcurl 초기화
    curl_global_init(CURL_GLOBAL_ALL);
    CURL* curl = curl_easy_init();

    if (curl) 
    {
        std::cout << "타일 생성을 시작합니다... (Zoom: " << zoom << ")" << std::endl;

        for (int x = startX; x <= endX; ++x) 
        {
            for (int y = startY; y <= endY; ++y) 
            {
                double minLon, minLat, maxLon, maxLat;
                tileToBBox(zoom, x, y, minLon, minLat, maxLon, maxLat);

                // 경로 및 파일명 설정
                std::string dirPath = "tiles/" + std::to_string(zoom) + "/" + std::to_string(x);
                fs::create_directories(dirPath);
                std::string filePath = dirPath + "/" + std::to_string(y) + ".png";

                // URL 구성 (WMS 1.1.1) 
                char url[1024];
                sprintf(url, "%s?map=%s&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
                    "&LAYERS=height&MODE=tile&SRS=EPSG:4326&WIDTH=256&HEIGHT=256" 
                    "&FORMAT=image/png&BBOX=%.8f,%.8f,%.8f,%.8f",
                    baseUrl.c_str(), mapFile.c_str(), minLon, minLat, maxLon, maxLat);

                FILE* fp = fopen(filePath.c_str(), "wb");
                if (fp) 
                {
                    curl_easy_setopt(curl, CURLOPT_URL, url);
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

                    // 요청 실행
                    CURLcode res = curl_easy_perform(curl);
                    fclose(fp);

                    if (res == CURLE_OK) 
                    {
                        std::cout << "[성공] " << filePath << std::endl;
                    }
                    else 
                    {
                        std::cerr << "[실패] " << filePath << ": " << curl_easy_strerror(res) << std::endl;
                    }
                }
            }
        }
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return 0;
}
