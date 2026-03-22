#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <algorithm>
#include "gdal_priv.h"


class CGeoidGrid
{
public:
    
    float cubicInterpolate(float p[4], float x);                // Cubic Hermite spline 커널 함수

    bool load(const std::string& filename);
    float getPixel(int x, int y);                               // 특정 픽셀 좌표의 값을 가져오되 경계 처리(Clamp) 포함
    float getCubicgeoidHeight(double lon, double lat);        // Bicubic 보간을 적용한 고도 추출
    double getLinergeoidHeight(double lat, double lon);    // 선형 보간을 사용

public:
    int width, height, maxValue;
    float scale = 0.003f;
    float offset = -107.0f;
    std::vector<unsigned char> data;
};


/*
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <fstream>

// Cubic Hermite spline 커널
float cubicInterpolate(float p[4], float x) {
    return p[1] + 0.5f * x * (p[2] - p[0] + x * (2.0f * p[0] - 5.0f * p[1] + 4.0f * p[2] - p[3] + x * (3.0f * (p[1] - p[2]) + p[3] - p[0])));
}

class EGM2008Manager {
public:
    int width = 21600;  // 1분 해상도 기준 (360 * 60)
    int height = 10801; // (180 * 60) + 1
    float scale = 0.01f; // EGM2008 16비트 PGM의 일반적인 스케일 (cm 단위 저장 시)
    float offset = -107.0f;
    std::vector<unsigned short> data;

    bool loadPGM(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) return false;

        std::string magic;
        int maxVal;
        file >> magic >> width >> height >> maxVal;
        file.ignore(); // 개행 무시

        data.resize(width * height);
        // 16비트 데이터이므로 바이트 오더링 주의 (Big-endian이 일반적)
        file.read(reinterpret_cast<char*>(data.data()), data.size() * 2);
        return true;
    }

    float getPixel(int x, int y) {
        x = (x + width) % width; // 경도 방향 무한 루프 (0도=360도)
        y = std::clamp(y, 0, height - 1);
        return static_cast<float>(data[y * width + x]) * scale + offset;
    }

    float getEllipsoidalHeight(double lon, double lat, float orthometricH) {
        // 위경도를 픽셀 좌표로 변환
        float u = (lon < 0 ? lon + 360 : lon) * (width / 360.0f);
        float v = (90.0f - lat) * ((height - 1) / 180.0f);

        int i = std::floor(u);
        int j = std::floor(v);
        float du = u - i;
        float dv = v - j;

        // Bicubic 4x4 샘플링
        float rowResults[4];
        for (int row = 0; row < 4; ++row) {
            float p[4];
            for (int col = 0; col < 4; ++col) {
                p[col] = getPixel(i - 1 + col, j - 1 + row);
            }
            rowResults[row] = cubicInterpolate(p, du);
        }

        float N = cubicInterpolate(rowResults, dv);
        return orthometricH + N;
    }
};

int main() {
    EGM2008Manager egm;
    if (!egm.loadPGM("und_ww15mgh_abs.pgm")) { // EGM2008 공식 PGM 파일명 예시
        std::cerr << "EGM2008 데이터를 로드할 수 없습니다." << std::endl;
        return 1;
    }

    // 예시: 서울 위경도 및 표고
    double lon = 127.0246;
    double lat = 37.5326;
    float H = 42.5f; // GeoTIFF에서 추출한 값

    float h = egm.getEllipsoidalHeight(lon, lat, H);

    std::cout.precision(6);
    std::cout << "표고 (H): " << H << "m" << std::endl;
    std::cout << "타원체고 (h): " << h << "m" << std::endl;

    return 0;
}
*/