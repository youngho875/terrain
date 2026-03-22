//#include <iostream>
//#include <gdal_priv.h>
//#include <ogr_spatialref.h>

/**
 * @brief GeoTIFF 파일에서 특정 위도/경도 위치의 픽셀 값(지오이드고 N)을 읽어옵니다.
 * * @param geotiff_path GeoTIFF 파일 경로
 * @param lon 경도 (degrees)
 * @param lat 위도 (degrees)
 * @return double 지오이드고 N 값 (미터), 실패 시 -9999.0 반환
 */

/*
double get_geoid_height_from_geotiff(const char* geotiff_path, double lon, double lat) {
    // 1. GDAL 초기화 및 데이터셋 열기
    GDALAllRegister();
    GDALDataset* poDataset = (GDALDataset*)GDALOpen(geotiff_path, GA_ReadOnly);
    if (poDataset == NULL) {
        std::cerr << "ERROR: Failed to open GeoTIFF file: " << geotiff_path << std::endl;
        return -9999.0;
    }

    // 2. 밴드(데이터) 가져오기 및 데이터 타입 확인
    GDALRasterBand* poBand = poDataset->GetRasterBand(1);
    int nXSize = poDataset->GetRasterXSize();
    int nYSize = poDataset->GetRasterYSize();

    // 3. 지리 변환(GeoTransform) 정보 가져오기
    double adfGeoTransform[6];
    if (poDataset->GetGeoTransform(adfGeoTransform) != CE_None) {
        std::cerr << "ERROR: Failed to get GeoTransform from GeoTIFF." << std::endl;
        GDALClose(poDataset);
        return -9999.0;
    }

    // 4. 좌표계 변환 설정 (입력 좌표 -> GeoTIFF 래스터 좌표)
    OGRSpatialReference oSourceSRS, oTargetSRS;

    // 입력 좌표계: WGS84 위경도 (EPSG:4326)
    oSourceSRS.SetWellKnownGeogCS("WGS84");

    // 대상 좌표계: GeoTIFF 파일의 좌표계
    const char* pszWKT = poDataset->GetProjectionRef();
    if (oTargetSRS.importFromWkt(&pszWKT) != OGRERR_NONE) {
        std::cerr << "ERROR: Failed to import GeoTIFF WKT." << std::endl;
        GDALClose(poDataset);
        return -9999.0;
    }

    // 좌표 변환 객체 생성
    OGRCoordinateTransformation* poCT = OGRCreateCoordinateTransformation(&oSourceSRS, &oTargetSRS);
    if (poCT == NULL) {
        std::cerr << "ERROR: Failed to create coordinate transformation object." << std::endl;
        GDALClose(poDataset);
        return -9999.0;
    }

    // 5. 위도/경도(Lon/Lat)를 GeoTIFF 맵 좌표(Map X/Y)로 변환
    // poCT->Transform은 내부적으로 InPlace 변환을 수행
    double mapX = lon;
    double mapY = lat;
    if (!poCT->Transform(1, &mapX, &mapY)) {
        std::cerr << "ERROR: Coordinate transformation failed." << std::endl;
        delete poCT;
        GDALClose(poDataset);
        return -9999.0;
    }

    // 6. 맵 좌표(Map X/Y)를 픽셀/라인 좌표(Pixel/Line)로 변환
    // Inverse GeoTransform 적용: P = (MapX - GT[0]) / GT[1]
    double pixel = (mapX - adfGeoTransform[0]) / adfGeoTransform[1];
    double line = (mapY - adfGeoTransform[3]) / adfGeoTransform[5];
    // GT[5]는 n-s 해상도로 일반적으로 음수입니다.

    int iCol = static_cast<int>(std::round(pixel));
    int iRow = static_cast<int>(std::round(line));

    // 7. 경계 검사
    if (iCol < 0 || iCol >= nXSize || iRow < 0 || iRow >= nYSize) {
        std::cerr << "WARNING: Coordinate is outside of GeoTIFF boundaries." << std::endl;
        delete poCT;
        GDALClose(poDataset);
        return -9999.0;
    }

    // 8. 픽셀 값 읽기 (지오이드고 N)
    float fPixelValue;
    poBand->RasterIO(GF_Read, iCol, iRow, 1, 1, &fPixelValue, 1, 1, GDT_Float32, 0, 0); 

    // 9. 정리 및 반환
    delete poCT;
    GDALClose(poDataset);

    return static_cast<double>(fPixelValue);
}

// --- 메인 함수에서 정표고 계산 ---
int main() {
    // 1. 입력 데이터
    const char* geoid_geotiff_path = "d:/mapdata/gebco_08_rev_elev_D1_grey_geo.tif"; // 지오이드 GeoTIFF 파일 경로 (예시)

    // 타원체고 (GPS 장치에서 얻은 값이라고 가정, WGS84 기준)
    double ellipsoidal_height_h = 150.75; // 예시: 150.75 m

    // 좌표 (대한민국 서울 예시)
    double latitude = 37.5665;  // 위도
    double longitude = 126.9780; // 경도

    // 2. GeoTIFF에서 지오이드고 N 값 가져오기
    double geoid_height_N = get_geoid_height_from_geotiff(
        geoid_geotiff_path,
        longitude,
        latitude
    );

    // 3. 정표고 계산 (H = h - N)
    if (geoid_height_N > -9999.0) {
        double orthometric_height_H = ellipsoidal_height_h - geoid_height_N;

        std::cout << "--- Orthometric Height Calculation Result ---" << std::endl;
        std::cout << "Input Ellipsoidal Height (h): " << ellipsoidal_height_h << " m" << std::endl;
        std::cout << "Geoid Height (N) from GeoTIFF: " << geoid_height_N << " m" << std::endl;
        std::cout << "Calculated Orthometric Height (H): " << orthometric_height_H << " m" << std::endl;
    }
    else {
        std::cerr << "Failed to calculate orthometric height due to error in reading GeoTIFF." << std::endl;
    }

    // GDAL 라이브러리 정리 (선택적)
    // GDALDestroyDriverManager();

    return 0;
}
*/

//#include <iostream>
//#include <gdal_priv.h>
//#include <ogr_spatialref.h>

/**
 * @brief GeoTIFF (.dem) 파일을 열고 특정 위도/경도 위치의 고도를 읽는 함수
 * * @param dem_path DEM 파일 경로
 * @param lon 경도 (degrees, WGS84 가정)
 * @param lat 위도 (degrees, WGS84 가정)
 * @return double 해당 위치의 고도 값, 실패 시 -9999.0 반환
 */
/*
double read_dem_elevation(const char* dem_path, double lon, double lat) {
    // 1. GDAL 초기화 및 데이터셋 열기
    GDALAllRegister();
    GDALDataset* poDataset = (GDALDataset*)GDALOpen(dem_path, GA_ReadOnly);

    if (poDataset == NULL) {
        std::cerr << "ERROR: Failed to open DEM file: " << dem_path << std::endl;
        return -9999.0;
    }

    // 파일 정보 출력
    int nXSize = poDataset->GetRasterXSize();
    int nYSize = poDataset->GetRasterYSize();
    std::cout << "DEM Loaded: " << dem_path << std::endl;
    std::cout << "Raster Size: " << nXSize << " x " << nYSize << std::endl;

    // 2. 밴드 (고도 데이터) 가져오기
    GDALRasterBand* poBand = poDataset->GetRasterBand(1);

    // 3. 지리 변환 정보 가져오기
    double adfGeoTransform[6];
    if (poDataset->GetGeoTransform(adfGeoTransform) != CE_None) {
        std::cerr << "ERROR: Failed to get GeoTransform." << std::endl;
        GDALClose(poDataset);
        return -9999.0;
    }

    // 4. 좌표계 변환 설정 (WGS84 위경도 -> DEM 파일의 좌표계)
    OGRSpatialReference oSourceSRS, oTargetSRS;
    oSourceSRS.SetWellKnownGeogCS("WGS84"); // 입력 좌표계 (WGS84 위경도 가정)

    const char* pszWKT = poDataset->GetProjectionRef();
    if (oTargetSRS.importFromWkt(&pszWKT) != OGRERR_NONE) {
        std::cerr << "ERROR: Failed to import DEM WKT." << std::endl;
        GDALClose(poDataset);
        return -9999.0;
    }

    OGRCoordinateTransformation* poCT = OGRCreateCoordinateTransformation(&oSourceSRS, &oTargetSRS);
    if (poCT == NULL) {
        std::cerr << "ERROR: Failed to create coordinate transformation object." << std::endl;
        GDALClose(poDataset);
        return -9999.0;
    }

    // 5. 위도/경도(Lon/Lat)를 DEM 맵 좌표(Map X/Y)로 변환
    double mapX = lon;
    double mapY = lat;
    if (!poCT->Transform(1, &mapX, &mapY)) {
        std::cerr << "ERROR: Coordinate transformation failed." << std::endl;
        delete poCT;
        GDALClose(poDataset);
        return -9999.0;
    }

    // 6. 맵 좌표(Map X/Y)를 픽셀/라인 좌표(Pixel/Line)로 변환 (역변환 공식)
    // 이 공식은 회전되지 않은(GT[2]=0, GT[4]=0) DEM에만 적용됩니다.
    double pixel = (mapX - adfGeoTransform[0]) / adfGeoTransform[1];
    double line = (mapY - adfGeoTransform[3]) / adfGeoTransform[5];

    int iCol = static_cast<int>(std::round(pixel));
    int iRow = static_cast<int>(std::round(line));

    // 7. 경계 검사
    if (iCol < 0 || iCol >= nXSize || iRow < 0 || iRow >= nYSize) {
        std::cerr << "WARNING: Coordinate (" << lon << ", " << lat << ") is outside of DEM boundaries." << std::endl;
        delete poCT;
        GDALClose(poDataset);
        return -9999.0;
    }

    // 8. 픽셀 값 읽기 (고도)
    float fPixelValue;
    // RasterIO(액세스 타입, 시작 x, 시작 y, 읽을 크기 x, 읽을 크기 y, 버퍼, 버퍼 x 크기, 버퍼 y 크기, 타입, x 스킵, y 스킵)
    poBand->RasterIO(GF_Read, iCol, iRow, 1, 1, &fPixelValue, 1, 1, GDT_Float32, 0, 0);

    // 9. 정리 및 반환
    delete poCT;
    GDALClose(poDataset);

    std::cout << "Reading elevation at (Lon: " << lon << ", Lat: " << lat << ")" << std::endl;
    std::cout << "Pixel Index (Col: " << iCol << ", Row: " << iRow << ")" << std::endl;

    return static_cast<double>(fPixelValue);
}

// --- 메인 함수 실행 예시 ---
int main() {
    // 실제 .dem 파일 경로로 변경하세요.
    const char* dem_file_path = "d:/mapdata/output.dem";

    // 고도를 확인하고 싶은 특정 좌표 (예: 서울)
    double target_lat = 37.5665;
    double target_lon = 126.9780;

    // 고도 읽기
    double elevation = read_dem_elevation(
        dem_file_path,
        target_lon,
        target_lat
    );

    if (elevation > -9999.0) {
        std::cout << "\nElevation at the point: " << elevation << " meters" << std::endl;
    }
    else {
        std::cerr << "\nFailed to retrieve elevation." << std::endl;
    }

    // GDAL 라이브러리 정리 (선택적)
    // GDALDestroyDriverManager();

    return 0;
}
*/

#include <iostream>
#include<string>
#include <gdal_priv.h>
#include <ogr_spatialref.h>

/**
 * @brief AAIGrid (.asc 또는 .dem) 파일을 열고 특정 위도/경도 위치의 고도를 읽는 함수
 * * @param dem_path AAIGrid 파일 경로
 * @param lon 경도 (degrees, WGS84 가정)
 * @param lat 위도 (degrees, WGS84 가정)
 * @return double 해당 위치의 고도 값, 실패 시 -9999.0 또는 NoDataValue 반환
 */
double read_aai_dem_elevation(std::string dem_path, double lon, double lat) {
    // 1. GDAL 초기화 및 데이터셋 열기
    GDALAllRegister();
    GDALDataset* poDataset = (GDALDataset*)GDALOpen(dem_path.c_str(), GA_ReadOnly);

    if (poDataset == NULL) {
        std::cerr << "ERROR: Failed to open AAIGrid DEM file: " << dem_path << std::endl;
        return -9999.0;
    }

    // 파일 정보 확인
    int nXSize = poDataset->GetRasterXSize();
    int nYSize = poDataset->GetRasterYSize();

    std::cout << "DEM Loaded: " << dem_path << std::endl;
    std::cout << "Raster Size: " << nXSize << " x " << nYSize << std::endl;

    // 2. 밴드 (고도 데이터) 가져오기 및 NoData 값 확인
    GDALRasterBand* poBand = poDataset->GetRasterBand(1);
    int bGotNoData;
    double dfNoData = poBand->GetNoDataValue(&bGotNoData);

    // 3. 지리 변환 정보 가져오기
    double adfGeoTransform[6];
    if (poDataset->GetGeoTransform(adfGeoTransform) != CE_None) {
        std::cerr << "ERROR: Failed to get GeoTransform." << std::endl;
        GDALClose(poDataset);
        return -9999.0;
    }

    // 4. 좌표계 변환 설정 (WGS84 위경도 -> DEM 파일의 좌표계)
    OGRSpatialReference oSourceSRS, oTargetSRS;
    oSourceSRS.SetWellKnownGeogCS("WGS84"); // 입력 좌표계 (WGS84 위경도 가정)

    const char* pszWKT = poDataset->GetProjectionRef();
    if (oTargetSRS.importFromWkt(&pszWKT) != OGRERR_NONE) {
        std::cerr << "ERROR: Failed to import DEM WKT." << std::endl;
        GDALClose(poDataset);
        return -9999.0;
    }

    OGRCoordinateTransformation* poCT = OGRCreateCoordinateTransformation(&oSourceSRS, &oTargetSRS);
    if (poCT == NULL) {
        std::cerr << "ERROR: Failed to create coordinate transformation object." << std::endl;
        GDALClose(poDataset);
        return -9999.0;
    }

    // 5. 위도/경도(Lon/Lat)를 DEM 맵 좌표(Map X/Y)로 변환
    double mapX = lon;
    double mapY = lat;
    if (!poCT->Transform(1, &mapX, &mapY)) {
        std::cerr << "ERROR: Coordinate transformation failed." << std::endl;
        delete poCT;
        GDALClose(poDataset);
        return -9999.0;
    }

    // 6. 맵 좌표(Map X/Y)를 픽셀/라인 좌표(Pixel/Line)로 변환
    // Inverse GeoTransform 적용
    double pixel = (mapX - adfGeoTransform[0]) / adfGeoTransform[1];
    double line = (mapY - adfGeoTransform[3]) / adfGeoTransform[5];

    int iCol = static_cast<int>(std::round(pixel));
    int iRow = static_cast<int>(std::round(line));

    // 7. 경계 검사
    if (iCol < 0 || iCol >= nXSize || iRow < 0 || iRow >= nYSize) {
        std::cerr << "WARNING: Coordinate is outside of DEM boundaries." << std::endl;
        delete poCT;
        GDALClose(poDataset);
        return -9999.0;
    }

    // 8. 픽셀 값 읽기 (고도)
    float fPixelValue;
    poBand->RasterIO(GF_Read, iCol, iRow, 1, 1, &fPixelValue, 1, 1, GDT_Float32, 0, 0);

    // 9. NoData 값 처리
    if (bGotNoData && fPixelValue == static_cast<float>(dfNoData)) {
        std::cout << "The point is on a NoData area." << std::endl;
        delete poCT;
        GDALClose(poDataset);
        return -9999.0; // 또는 사용자 정의 NoData 값 반환
    }

    // 10. 정리 및 반환
    delete poCT;
    GDALClose(poDataset);

    return static_cast<double>(fPixelValue);
}

// --- 메인 함수 실행 예시 ---
int main() {
    // 실제 AAIGrid 파일 경로로 변경하세요. (.asc 또는 .dem)
    std::string dem_file_path = "d:/mapdata/height/height.asc";

    // 고도를 확인하고 싶은 특정 좌표 (예: 서울)
    double target_lat = 37.5665;
    double target_lon = 126.9780;

    // 고도 읽기
    double elevation = read_aai_dem_elevation(
        dem_file_path,
        target_lon,
        target_lat
    );

    if (elevation > -9999.0) {
        std::cout << "\nElevation at the point: " << elevation << " meters" << std::endl;
    }
    else {
        std::cerr << "\nFailed to retrieve elevation or hit NoData area." << std::endl;
    }

    return 0;
}