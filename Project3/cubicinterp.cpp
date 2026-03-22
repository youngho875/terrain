#include "GeoidGrid.h"

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

    CGeoidGrid geoid;
    
    std::ifstream pgmFile("D:/mapdata/geoids/egm96-5.pgm", std::ios::binary);
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
    float N = geoid.getCubicgeoidHeight(lon, lat);
    float h = H + N;

    // real height
    // H = h - N

    std::cout << "-------- Bicubic Interpolated -----------------" << std::endl;
    std::cout << "표고(H): " << H << "m" << std::endl;
    std::cout << "지오이드고(N): " << N << "m" << std::endl;
    std::cout << "타원체고(h): " << h << "m" << std::endl;
    std::cout << "-----------------------------------------------" << std::endl;

    return 0;
}

// egm2008-5.pgm
//int width = 21600;  // 1분 해상도 기준 (360 * 60)
//int height = 10801; // (180 * 60) + 1
//float scale = 0.01f; // EGM2008 16비트 PGM의 일반적인 스케일 (cm 단위 저장 시)
//float offset = -107.0f;