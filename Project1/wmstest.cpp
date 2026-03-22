#include <algorithm>
#include <iostream>
#include <string>
#include "curl/curl.h"
#include <fstream>
#include <vector>
#include <cmath>


/*
// cURL 콜백 함수
size_t writeData(void* ptr, size_t size, size_t nmemb, std::ofstream* stream) 
{
    size_t totalSize = size * nmemb;
    stream->write(static_cast<char*>(ptr), totalSize);
    return totalSize;
}

// WMS 서비스에서 이미지 받아오기
bool fetchWMSImage(const std::string& wmsUrl, const std::string& params, const std::string& outputFilename)
{
    CURL* curl;
    CURLcode res;
    std::ofstream outFile(outputFilename, std::ios::binary);

    if (!outFile) {
        std::cerr << "Error opening file: " << outputFilename << std::endl;
        return false;
    }

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, (wmsUrl + "?" + params).c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData); 
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outFile);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // 리다이렉션을 따르도록 설정
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            outFile.close();
            return false;
        }
        curl_easy_cleanup(curl);
    }
    outFile.close();
    return true;
}

// 시스템의 기본 이미지 뷰어로 이미지 열기
void openImage(const std::string& filename) {
    system(("start " + filename).c_str()); // Windows
}

int main() 
{
    // WMS 서비스 URL
    std::string wmsUrl = "http://10.240.33.120/cgi-bin/mapserv.exe"; // 실제 WMS 서비스 URL로 변경

    // WMS 요청 파라미터 설정
    std::string params = "map=/ms4w/apps/local-demo/ne1_hr.map"
        "&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
        "&LAYERS=NE1_HR" // 실제 레이어 이름으로 변경
        "@STYLES"
        "&BBOX=-180.0,-90.0,180.0,90.0" // Bounding box (실제 값으로 변경)
        "&WIDTH=800&HEIGHT=600"
        "&FORMAT=image/png"
        "&CRS=EPSG:4326"; // CRS

    // 출력 파일 이름
    std::string outputFilename = "D:/ms4w/tmp/wms_image.png";

    // 이미지 가져오기
    if (fetchWMSImage(wmsUrl, params, outputFilename)) {
        std::cout << "Image saved as " << outputFilename << std::endl;
        openImage(outputFilename); // 기본 뷰어로 이미지 열기
    }
    else {
        std::cerr << "Failed to fetch the image." << std::endl;
    }

    return 0;
}

inline double Deg2Rad(double deg) {
    return deg * M_PI / 180.0;
}

inline double Rad2Deg(double rad) {
    return rad * 180.0 / M_PI;
}

#include <iostream>
#include <string>
#include <curl/curl.h>
#include <cmath>
#include <vector>
#include <fstream>
*/



#define     M_PI        3.14195
#define     DEG2RAD     (M_PI / 180.0)

// cURL 콜백 함수
size_t writeData(void* ptr, size_t size, size_t nmemb, std::ofstream* stream) 
{
    size_t totalSize = size * nmemb;
    stream->write(static_cast<char*>(ptr), totalSize);
    return totalSize;
}

// WMS 서비스에서 타일 이미지를 받아오기
bool fetchWMSTile(const std::string& wmsUrl, const std::string& layers,
    double minLon, double minLat, double maxLon, double maxLat,
    int width, int height, const std::string& outputFile) 
{
    CURL* curl;
    CURLcode res;
    std::ofstream outFile(outputFile, std::ios::binary);

    if (!outFile) 
    {
        std::cerr << "Error opening file: " << outputFile << std::endl;
        return false;
    }

    // WMS 요청 파라미터
    std::string params =  "map=/ms4w/apps/local-demo/height.map"
        "&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
        "&MODE=map"
        "&LAYERS=" + layers +
        "&BBOX=" + std::to_string(minLon) + "," + std::to_string(minLat) + "," +
        std::to_string(maxLon) + "," + std::to_string(maxLat) +
        "&WIDTH=" + std::to_string(width) +
        "&HEIGHT=" + std::to_string(height) +
        "&STYLE=" +
        "&FORMAT=image/png&CRS=EPSG:4326";

    // cURL 초기화
    curl = curl_easy_init();
    if (curl) 
    {
        curl_easy_setopt(curl, CURLOPT_URL, (wmsUrl + "?" + params).c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outFile);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        // 요청 수행
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) 
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            outFile.close();
            return false;
        }
        curl_easy_cleanup(curl);
    }
    outFile.close();
    return true;
}

// 시스템의 기본 이미지 뷰어로 이미지 열기
void openImage(const std::string& filename) {
    system(("start " + filename).c_str()); // Windows
}

// 타일을 부탁 받고 그림
void createTiles(const std::string& wmsUrl, const std::string& layers,
    double minLon, double minLat, double maxLon, double maxLat,
    int tileWidth, int tileHeight, int zoomLevel, int rows, int cols) 
{
    double lonStep = (maxLon - minLon) / cols;
    double latStep = (maxLat - minLat) / rows;

    for (int i = 0; i < rows; ++i) 
    {
        for (int j = 0; j < cols; ++j) 
        {
            double tileMinLon = minLon + j * lonStep;
            double tileMaxLon = minLon + (j + 1) * lonStep;
            double tileMinLat = maxLat - (i + 1) * latStep; // Y축은 반전
            double tileMaxLat = maxLat - i * latStep;

            std::string outputFile = "tile_" + std::to_string(zoomLevel) + "_" + std::to_string(i) + "_" + std::to_string(j) + ".png";
             if (fetchWMSTile(wmsUrl, layers, tileMinLon, tileMinLat, tileMaxLon, tileMaxLat, tileWidth, tileHeight, outputFile)) 
             {
                std::cout << "Tile saved: " << outputFile << std::endl;
                //openImage(outputFile);
             }
            else 
            {
                std::cerr << "Failed to fetch tile: " << outputFile << std::endl;
            }
        }
    }
}

/*
int computeZoomLevel(double targetResolutionMeters, double latitudeDeg)
{
    constexpr double earthCircumference = 40075016.68557849;

    double latRad = latitudeDeg * M_PI / 180.0;
    double adjustedResolution = targetResolutionMeters / std::cos(latRad);

    return static_cast<int> (
        std::log2(earthCircumference / adjustedResolution)
    );
}

void lonLatToTileXY(double lon, double lat, int level, int& x, int& y)
{
    int n = 1 << level;

    x = int((lon + 180.0) / 360.0 * n);

    double latRad = lat * M_PI / 180.0;
    y = int((1.0 - log( tan(latRad) + 1.0/cos(latRad))/M_PI)/2.0 * n);
}
*/


int computeZoomLevel(double minLat, double minLon, double maxLat, double maxLon, double targetResolutionMeters)
{
    constexpr double EarthCircumference = 40075016.6856;

    double centerLat = (minLat + maxLat) * 0.5;
    double latRad = centerLat * M_PI / 180.0;

    double metersPerTileAtZ0 = EarthCircumference * cos(latRad);

    double z = log2(metersPerTileAtZ0 / targetResolutionMeters);

    int zoom = static_cast<int>(std::ceil(z));

    return std::clamp(zoom, 0, 20);
}

int lonToTileX(double lon, int zoom)
{
    int n = 1 << zoom;
    return static_cast<int>(floor((lon + 180.0) / 360.0 * n));
}

int latToTileY(double lat, int zoom) 
{
    double latRad = lat * DEG2RAD;
    int n = 1 << zoom;

    return static_cast<int>(floor((1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / M_PI) / 2.0 * n));
}



int main() 
{
    std::string wmsUrl = "http://10.240.33.120/cgi-bin/mapserv.exe"; // 실제 WMS URL로 변경
    std::string layers = "height"; // 사용할 레이어 이름으로 변경
     
    // 위경도 범위 설정
    //double minLon = -74.0;      // 최소 경도
    //double minLat = 40.0;       // 최소 위도
    //double maxLon = -73.0;      // 최대 경도
    //double maxLat = 41.0;       // 최대 위도

    double minLon = -180.0;      // 최소 경도
    double minLat = -90.0;       // 최소 위도
    double maxLon = 180.0;      // 최대 경도 
    double maxLat = 90.0;       // 최대 위도


    // 타일 크기 및 줌 레벨
    //int tileWidth = 256;
    //int tileHeight = 256;
    //int zoomLevel = 0;          // 줌 레벨
    //int rows = 1;               // 생성할 행 수
    //int cols = 1;               // 생성할 열 수 

    //int rows, cols; 
    int zoomLevel = computeZoomLevel(minLat, minLon, maxLat, maxLon, 100);
    //int x = lonToTileX(minLon, zoomLevel);
    //int y = latToTileY(maxLat, zoomLevel);

    //std::cout << zoomLevel << "," << x << "," << y << std::endl;
    //lonLatToTileXY(maxLon, maxLat, zoomLevel, cols, rows);
       

    // 타일 생성
    //createTiles(wmsUrl, layers, minLon, minLat, maxLon, maxLat, tileWidth, tileHeight, zoomLevel, rows, cols);

    return 0;
}
