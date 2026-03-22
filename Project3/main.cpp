#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include "gdal_priv.h"

class EGM96Reader
{
private:
    std::vector<int16_t> gridData;
    int width, height, maxValue;
    double offset, scale;

public:
    // .pgm 파일을 읽어 메모리에 로드
    bool load(const std::string& filename) 
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file) return false;

        std::string header;
        file >> header; // P5 확인
        if (header != "P5") return false;

        // PGM 헤더 파싱 (주석 제외)
        auto skipComments = [&](std::ifstream& f) {
            char ch;
            while (f.peek() == '#' || isspace(f.peek())) {
                if (f.peek() == '#') f.ignore(1000, '\n');
                else f.get(ch);
            }
        };

        skipComments(file); file >> width;
        skipComments(file); file >> height;
        skipComments(file); file >> maxValue;
        file.ignore(1); // 헤더 끝 공백 무시

        // 데이터 로드 (2바이트 Big-Endian)
        gridData.resize(width * height);
        for (int i = 0; i < width * height; ++i) 
        {
            unsigned char bytes[2];
            file.read((char*)bytes, 2);

            // Big-Endian을 Native 정수로 변환
            gridData[i] = (int16_t)((bytes[0] << 8) | bytes[1]);
        }

        // EGM96 PGM 표준 오프셋 및 스케일 (보통 -108m 오프셋, 0.01 스케일)
        // 이는 파일 내부 메타데이터가 아니라 EGM96 PGM 규격 정의값입니다.
        offset = -108.0;
        scale = 0.01;

        return true;
    }

    // 선형 보간을 사용하여 지오이드 높이 계산
    double getGeoidHeight(double lat, double lon) 
    {
        if (lon < 0) lon += 360.0; // 경도 0~360 범위로 보정

        // 격자 좌표 계산 (EGM96 5분 격자 기준)
        // 위도: 90(0행) -> -90(height-1행)
        // 경도: 0(0열) -> 360(width-1열)
        double c = lon * (width - 1) / 360.0;
        double r = (90.0 - lat) * (height - 1) / 180.0;

        int c0 = (int)std::floor(c);
        int r0 = (int)std::floor(r);
        int c1 = std::min(c0 + 1, width - 1);
        int r1 = std::min(r0 + 1, height - 1);

        double dr = r - r0;
        double dc = c - c0;

        // 쌍선형 보간 (Bilinear Interpolation)
        double v00 = gridData[r0 * width + c0];
        double v10 = gridData[r1 * width + c0];
        double v01 = gridData[r0 * width + c1];
        double v11 = gridData[r1 * width + c1];

        double v0 = v00 * (1 - dr) + v10 * dr;
        double v1 = v01 * (1 - dr) + v11 * dr;
        double rawVal = v0 * (1 - dc) + v1 * dc;

        return (rawVal * scale) + offset;
    }
};

/*
int main() 
{
    EGM96Reader egm;
    
    if (!egm.load("D:/mapdata/geoids/egm96-5.pgm")) {
        std::cerr << "파일을 로드할 수 없습니다." << std::endl;
        return -1;
    }

    //double lat = 37.5665; // 서울 위도
    //double lon = 126.9780; // 서울 경도

    double lon = 128.0775; // 경도
    double lat = 41.9930;  // 위도

    //double lon = 86.92528; // 경도
    //double lat = 27.98833;  // 위도

    double n = egm.getGeoidHeight(lat, lon);
    std::cout << "추출된 지오이드 높이 (N): " << n << " meters" << std::endl;

    return 0;
}
*/


// PGM 데이터를 관리하는 구조체
struct GeoidGrid
{
    int width, height;
    float offset, scale; // PGM 값을 실제 미터(m) 단위로 변환하기 위한 계수
    std::vector<unsigned char> data;

    // 위경도를 입력받아 해당 지점의 N(지오이드고)을 반환
    float getGeoidHeight(double lon, double lat) 
    {
        // EGM96 PGM 기준: 경도 0~360, 위도 90~-90 매핑
        // 파일의 해상도에 따라 index 계산 공식이 달라짐 (예: 5분 그리드)
        int x = static_cast<int>((lon < 0 ? lon + 360 : lon) / 360.0 * width);
        int y = static_cast<int>((90.0 - lat) / 180.0 * height);

        unsigned char rawVal = data[y * width + x];
        // 실제 고도 = (raw * scale) + offset (파일 스펙에 따라 다름)
        return static_cast<float>(rawVal) * scale + offset;
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

    // 2. egm96-5.pgm 로드 (간략화된 로더)
    GeoidGrid geoid;
    std::ifstream pgmFile("egm96-5.pgm", std::ios::binary);
    // ... (PGM 헤더 읽기 생략: width, height 설정 필요) ...
    geoid.width = 4320; // 예시 크기
    geoid.height = 2161;
    geoid.scale = 0.01f;   // EGM96 PGM 파일의 일반적인 스케일링
    geoid.offset = -108.0f;
    geoid.data.resize(geoid.width * geoid.height);
    pgmFile.read(reinterpret_cast<char*>(geoid.data.data()), geoid.data.size());



    //EGM96Reader egm;
    //if (!egm.load("D:/mapdata/geoids/egm96-5.pgm")) {
    //    std::cerr << "파일을 로드할 수 없습니다." << std::endl;
    //    return -1;
    //}

    // 3. 타원체고(h) 계산
    float N = geoid.getGeoidHeight(lat, lon);
    //float N = egm.getGeoidHeight(lat, lon);
    float h = H + N;

    std::cout << "표고(H): " << H << "m" << std::endl;
    std::cout << "지오이드고(N): " << N << "m" << std::endl;
    std::cout << "타원체고(h): " << h << "m" << std::endl;

    GDALClose(poDS);
    return 0;
}
