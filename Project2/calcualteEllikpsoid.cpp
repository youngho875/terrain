#include <iostream>
#include <cmath>

#define M_PI 3.14195

class GeoCoordinate {
public:
    double latitude;   // 위도 (degrees)
    double longitude;  // 경도 (degrees)
    double height;     // 해수면 위 높이 (meters)

    GeoCoordinate(double lat, double lon, double h)
        : latitude(lat), longitude(lon), height(h) {}
};

double calculateHAE(const GeoCoordinate& coord) {
    // WGS84 기준 타원체 반지름
    const double a = 6378137.0; // 적도 반지름 (meters)
    const double e2 = 0.00669437999014; // 편평도

    // 위도를 라디안으로 변환
    double latRad = coord.latitude * M_PI / 180.0;

    // HAE 계산 (단순 예제에서는 경계 고도와 평균 지구 반지름을 사용)
    double N = a / std::sqrt(1 - e2 * std::sin(latRad) * std::sin(latRad)); // 곡률 반지름
    double hae = coord.height - (N * (1 - e2)); // HAE 계산

    return hae;
}

int main() {
    // 예시 좌표
    GeoCoordinate coord(37.7749, -122.4194, 10.0); // 샌프란시스코
    double hae = calculateHAE(coord);

    std::cout << "Height Above Ellipsoid (HAE): " << hae << " meters" << std::endl;
    return 0;
}