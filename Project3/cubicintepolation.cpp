#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <algorithm>
#include "gdal_priv.h"

// Cubic Hermite spline 커널 함수
float cubicInterpolate(float p[4], float x) 
{
    return p[1] + 0.5f * x * (p[2] - p[0] + x * (2.0f * p[0] - 5.0f * p[1] + 4.0f * p[2] - p[3] + x * (3.0f * (p[1] - p[2]) + p[3] - p[0])));
}

struct GeoidGrid {
    int width, height;
    float scale = 0.5f;
    float offset = -108.0f;
    std::vector<unsigned char> data;

    // 특정 픽셀 좌표의 값을 가져오되 경계 처리(Clamp) 포함
    float getPixel(int x, int y) {
        x = std::clamp(x, 0, width - 1);
        y = std::clamp(y, 0, height - 1);
        return static_cast<float>(data[y * width + x]) * scale + offset;
    }

    // Bicubic 보간을 적용한 고도 추출
    float getInterpolatedN(double lon, double lat) {
        // 1. 위경도를 PGM 픽셀 좌표(실수형)로 변환
        float u = (lon < 0 ? lon + 360 : lon) / 360.0f * (width - 1);
        float v = (90.0f - lat) / 180.0f * (height - 1);

        int i = std::floor(u);
        int j = std::floor(v);
        float du = u - i;
        float dv = v - j;

        // 2. 주변 4x4 픽셀 샘플링
        float rowResults[4];
        for (int row = 0; row < 4; ++row) {
            float p[4];
            for (int col = 0; col < 4; ++col) {
                p[col] = getPixel(i - 1 + col, j - 1 + row);
            }
            rowResults[row] = cubicInterpolate(p, du);
        }

        // 3. 최종 수직 방향 보간
        return cubicInterpolate(rowResults, dv);
    }
};

int main() 
{
    GDALAllRegister();

    // 1. GeoTIFF에서 표고(H) 추출
    GDALDataset* poDS = (GDALDataset*)GDALOpen("d:/mapdata/height/height.vrt", GA_ReadOnly);
    double adfGT[6];
    poDS->GetGeoTransform(adfGT);

    // 테스트 좌표 및 H 값 추출
    //double lon = 127.0, lat = 37.5;
    //double lon = 128.0775; // 경도
    //double lat = 41.9930;  // 위도

    double lon = 86.92528; // 경도
    double lat = 27.98833;  // 위도

    int px = (lon - adfGT[0]) / adfGT[1];
    int py = (lat - adfGT[3]) / adfGT[5];
    float H;
    poDS->GetRasterBand(1)->RasterIO(GF_Read, px, py, 1, 1, &H, 1, 1, GDT_Float32, 0, 0);

    GeoidGrid geoid;
    // ... PGM 파일 로드 및 데이터 채우기 (생략) ...
    std::ifstream pgmFile("egm96-5.pgm", std::ios::binary);
    // ... (PGM 헤더 읽기 생략: width, height 설정 필요) ...
    geoid.width = 4320; // 예시 크기
    geoid.height = 2161;
    geoid.scale = 0.01f;   // EGM96 PGM 파일의 일반적인 스케일링
    geoid.offset = -108.0f;
    geoid.data.resize(geoid.width * geoid.height);
    pgmFile.read(reinterpret_cast<char*>(geoid.data.data()), geoid.data.size());

    //double lon = 127.1234;
    //double lat = 37.5678;
    //float orthometricH = 45.0f; // GeoTIFF에서 읽어온 값

    // Bicubic 보간을 적용한 지오이드고(N)
    float N = geoid.getInterpolatedN(lon, lat);
    float h = H + N;

    std::cout << "Bicubic Interpolated N: " << N << "m" << std::endl;
    std::cout << "Final Ellipsoidal Height: " << h << "m" << std::endl;

    return 0;
}