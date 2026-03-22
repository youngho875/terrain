#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "gdal_priv.h"
#include <cpl_conv.h>

class GeoCoordinate {
public:
    double latitude;   // 위도
    double longitude;  // 경도
    double height;     // HAE

    GeoCoordinate(double lat, double lon, double h)
        : latitude(lat), longitude(lon), height(h) {}
};

// 지오이드 높이를 계산하는 함수
double getGeoidHeight(const std::string& filename, double latitude, double longitude) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return 0.0; // 기본값
    }

    std::string line;
    double closestHeight = 0.0; // 기본값

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        double lat, lon, height;
        if (iss >> lat >> lon >> height) {
            // 지오이드 높이가 가장 가까운 점을 찾기
            if (lat == latitude && lon == longitude) {
                closestHeight = height;
                break; // 일치하는 높이 찾으면 종료
            }
        }
    }

    return closestHeight;
}

// GeoTIFF 파일에서 HAE를 읽어오는 함수
double readHAEFromGeoTIFF(const std::string& filename, double latitude, double longitude) {
    GDALAllRegister(); // GDAL 드라이버 등록
    GDALDataset* dataset = (GDALDataset*)GDALOpen(filename.c_str(), GA_ReadOnly);
    if (!dataset) {
        std::cerr << "Error opening GeoTIFF: " << filename << std::endl;
        return 0.0; // 기본값
    }

    // GeoTIFF 메타데이터 가져오기
    double geoTransform[6];
    dataset->GetGeoTransform(geoTransform);

    // GeoTIFF의 이미지 크기
    int rasterXSize = dataset->GetRasterXSize();
    int rasterYSize = dataset->GetRasterYSize();

    // 위도와 경도를 픽셀 좌표로 변환
    double pixelX = (longitude - geoTransform[0]) / geoTransform[1]; // X 좌표 (픽셀)
    double pixelY = (latitude - geoTransform[3]) / geoTransform[5]; // Y 좌표 (픽셀)

    // 픽셀 좌표가 유효한지 확인
    if (pixelX < 0 || pixelX >= rasterXSize || pixelY < 0 || pixelY >= rasterYSize) {
        std::cerr << "Error: Coordinates out of bounds." << std::endl;
        GDALClose(dataset);
        return 0.0; // 기본값
    }

    // HAE 값을 읽어오기 위해 데이터를 위한 배열
    float* buffer = (float*)CPLMalloc(sizeof(float) * rasterXSize * rasterYSize);

    // 데이터 읽기 (여기서는 첫 번째 밴드의 데이터를 읽는다고 가정)
    dataset->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, rasterXSize, rasterYSize, buffer,
        rasterXSize, rasterYSize, GDT_Float32, 0, 0);

    // 픽셀 좌표에서 HAE 추출
    double haeValue = buffer[static_cast<int>(pixelY) * rasterXSize + static_cast<int>(pixelX)];

    // 메모리 해제 및 파일 닫기
    CPLFree(buffer);
    GDALClose(dataset);

    return haeValue;
}

double calculateActualHeight(const std::string& geoidFile, const std::string& geoTiffFile, const GeoCoordinate& coord) {
    double geoidHeight = getGeoidHeight(geoidFile, coord.latitude, coord.longitude);
    double hae = readHAEFromGeoTIFF(geoTiffFile, coord.latitude, coord.longitude);
    double actualHeight = hae - geoidHeight; // 실제 고도 계산
    return actualHeight;
}

int main() {
    const std::string geoidFile = "D:/mapdata/GeoidHeights.dat"; // 데이터 파일 경로
    const std::string geoTiffFile = "D:/mapdata/gebco_08_rev_elev_D1_grey_geo.tif"; // GeoTIFF 파일 경로

    GeoCoordinate coord(39, 126, 10.0); // 예시 좌표
    double actualHeight = calculateActualHeight(geoidFile, geoTiffFile, coord);

    std::cout << "Actual Height Above Mean Sea Level: " << actualHeight << " meters" << std::endl;
    return 0;
}