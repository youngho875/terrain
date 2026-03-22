#include <iostream>
#include <string>
#include <cmath>
#include <limits>

// GDAL 및 OSR (Spatial Reference) 헤더 파일 포함
#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include "Altitude.h"


/**
 * 주어진 위경도 좌표에서 DEM 파일의 고도 값을 추출합니다.
 * * @param demFilePath DEM 파일 경로
 * @param longitude WGS84 경도
 * @param latitude WGS84 위도
 * @return 추출된 고도 값. 실패 시 -9999.0을 반환합니다.
 */
double getElevationFromLonLat(const std::string& demFilePath, double longitude, double latitude) 
{
    // 1. GDAL 데이터셋 열기
    GDALDataset* poDataset = (GDALDataset*)GDALOpen(demFilePath.c_str(), GA_ReadOnly);

    if (poDataset == nullptr) {
        std::cerr << "오류: DEM 파일을 열 수 없습니다: " << demFilePath << std::endl;
        return -9999.0;
    }

    // 2. 래스터 밴드(고도 데이터) 가져오기
    if (poDataset->GetRasterCount() < 1) {
        std::cerr << "오류: 래스터 밴드가 없습니다." << std::endl;
        GDALClose(poDataset);
        return -9999.0;
    }
    GDALRasterBand* poBand = poDataset->GetRasterBand(1);

    // 3. 좌표 변환 설정 (WGS84 -> DEM 좌표계)
    double mapX = longitude;
    double mapY = latitude;

    // DEM의 좌표계 정보 가져오기
    const char* pszProjection = poDataset->GetProjectionRef();

    // WGS84 (EPSG:4326)를 소스 좌표계로 설정
    OGRSpatialReference oSourceSRS;
    oSourceSRS.SetWellKnownGeogCS("WGS84");

    // DEM의 좌표계를 대상 좌표계로 설정
    OGRSpatialReference oTargetSRS;
    if (pszProjection != nullptr && strlen(pszProjection) > 0) {
        // DEM의 좌표계 문자열을 사용하여 설정
        oTargetSRS.importFromWkt(pszProjection);
    }
    else {
        // 투영 정보가 없으면 WGS84를 사용한다고 가정 (위험할 수 있음)
        oTargetSRS.SetWellKnownGeogCS("WGS84");
    }

    // 좌표 변환 객체 생성
    OGRCoordinateTransformation* poCT = OGRCreateCoordinateTransformation(&oSourceSRS, &oTargetSRS);

    // 좌표 변환 수행 (mapX/mapY가 DEM의 좌표계로 변환됨)
    if (poCT != nullptr) {
        if (!poCT->Transform(1, &mapX, &mapY)) {
            std::cerr << "오류: 좌표 변환 실패." << std::endl;
            delete poCT;
            GDALClose(poDataset);
            return -9999.0;
        }
        delete poCT;
    }
    else if (pszProjection != nullptr && strlen(pszProjection) > 0) {
        std::cerr << "경고: DEM 좌표계 변환 객체 생성 실패. 좌표 변환 없이 진행합니다." << std::endl;
    }

    // 4. 픽셀 위치(Col/Row) 계산
    double adfGeoTransform[6];
    if (poDataset->GetGeoTransform(adfGeoTransform) != CE_None) {
        std::cerr << "오류: GeoTransform 정보 가져오기 실패." << std::endl;
        GDALClose(poDataset);
        return -9999.0;
    }

    // 역 GeoTransform을 사용하여 지도 좌표(mapX, mapY)를 픽셀 좌표(Col, Row)로 변환
    double adfInvGeoTransform[6];
    if (!GDALInvGeoTransform(adfGeoTransform, adfInvGeoTransform)) {
        std::cerr << "오류: 역 GeoTransform 계산 실패." << std::endl;
        GDALClose(poDataset);
        return -9999.0;
    }

    double dCol = adfInvGeoTransform[0] + adfInvGeoTransform[1] * mapX + adfInvGeoTransform[2] * mapY;
    double dRow = adfInvGeoTransform[3] + adfInvGeoTransform[4] * mapX + adfInvGeoTransform[5] * mapY;

    int nCol = static_cast<int>(std::floor(dCol));
    int nRow = static_cast<int>(std::floor(dRow));

    // 픽셀 경계 검사
    if (nCol < 0 || nCol >= poDataset->GetRasterXSize() ||
        nRow < 0 || nRow >= poDataset->GetRasterYSize()) {
        std::cerr << "오류: 좌표가 래스터 범위(" << nCol << ", " << nRow << ")를 벗어났습니다." << std::endl;
        GDALClose(poDataset);
        return -9999.0;
    }

    // 5. 고도 값 읽기
    float elevation_buffer;
    CPLErr error = poBand->RasterIO(
        GF_Read,
        nCol,      // 읽기 시작 X 오프셋 (컬럼)
        nRow,      // 읽기 시작 Y 오프셋 (로우)
        1,         // 읽을 너비 (1 픽셀)
        1,         // 읽을 높이 (1 픽셀)
        &elevation_buffer, // 데이터를 저장할 버퍼
        1,         // 버퍼 너비 (1)
        1,         // 버퍼 높이 (1)
        GDT_Float32, // 메모리 버퍼 데이터 타입
        0,
        0
    );

    GDALClose(poDataset);

    if (error != CE_None) {
        std::cerr << "오류: RasterIO 실패." << std::endl;
        return -9999.0;
    }

    // 6. NODATA 값 처리
    int pbSuccess;
    double noDataValue = poBand->GetNoDataValue(&pbSuccess);
    if (pbSuccess && elevation_buffer == static_cast<float>(noDataValue)) {
        std::cout << "추출된 값은 NODATA 값입니다." << std::endl;
        return -9999.0;
    }

    return elevation_buffer;
}

// ----------------------------------------------------
// 메인 함수 (사용 예시)
// ----------------------------------------------------
int main() 
{
    // GDAL 라이브러리 초기화               
    GDALAllRegister();
    //OSRInitialize();

    // 📌 1. DEM 파일 경로 설정 (실제 경로로 변경하세요)
    std::string demFile = "d:/mapdata/height/height.vrt";

    // 📌 2. 추출을 원하는 위경도 좌표 설정 (WGS84)
    //백두산
    //double targetLon = 128.0775; // 경도
    //double targetLat = 41.9930;  // 위도

    // 에버레스트
    double targetLon = 86.92528; // 경도
    double targetLat = 27.98833;  // 위도

    CAltitude* mHeights;
    mHeights = NULL;

    std::cout << "DEM 파일: " << demFile << std::endl;
    std::cout << "좌표: (" << targetLon << ", " << targetLat << ")" << std::endl;

    double elevation = getElevationFromLonLat(demFile, targetLon, targetLat);

    if (elevation != -9999.0) {
        std::cout << "========================================" << std::endl;
        std::cout << "추출된 고도 값: " << elevation << " 미터" << std::endl;
        std::cout << "========================================" << std::endl;


        double geoidheight = mHeights->get_geoidHeight(targetLat, targetLon);
        std::cout << "========================================" << std::endl; 
        std::cout << "추출된 Geoid 고도 값: " << geoidheight << " 미터" << std::endl;
        std::cout << "========================================" << std::endl;

        double orthoheight = elevation - geoidheight;
        std::cout << "========================================" << std::endl;
        std::cout << "고도 값: " << orthoheight << " 미터" << std::endl;
        std::cout << "========================================" << std::endl;
    }
    else {
        std::cout << "고도 추출에 실패했거나 NODATA 값입니다." << std::endl;
    }

    // GDAL 라이브러리 종료
    GDALDestroyDriverManager();

    return 0;
}
