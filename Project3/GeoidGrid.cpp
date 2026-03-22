#include "GeoidGrid.h"

// Cubic Hermite spline 커널 함수
float CGeoidGrid::cubicInterpolate(float p[4], float x)
{
    return p[1] + 0.5f * x * (p[2] - p[0] + x * (2.0f * p[0] - 5.0f * p[1] + 4.0f * p[2] - p[3] + x * (3.0f * (p[1] - p[2]) + p[3] - p[0])));
}

float CGeoidGrid::getPixel(int x, int y) 
{
    x = std::clamp(x, 0, width - 1);
    y = std::clamp(y, 0, height - 1);
    return static_cast<float>(data[y * width + x]) * scale + offset;
}

// .pgm 파일을 읽어 메모리에 로드
bool CGeoidGrid::load(const std::string& filename)
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
    data.resize(width * height);
    for (int i = 0; i < width * height; ++i)
    {
        unsigned char bytes[2];
        file.read((char*)bytes, 2);

        // Big-Endian을 Native 정수로 변환
        data[i] = (int16_t)((bytes[0] << 8) | bytes[1]);
    }

    // EGM96 PGM 표준 오프셋 및 스케일 (보통 -108m 오프셋, 0.01 스케일)
    // 이는 파일 내부 메타데이터가 아니라 EGM96 PGM 규격 정의값입니다.
    offset = -108.0;
    scale = 0.01;

    return true;
}

// Bicubic 보간을 적용한 고도 추출
float CGeoidGrid::getCubicgeoidHeight(double lon, double lat) 
{
    // 1. 위경도를 PGM 픽셀 좌표(실수형)로 변환
    float u = (lon < 0 ? lon + 360 : lon) / 360.0f * (width - 1);
    float v = (90.0f - lat) / 180.0f * (height - 1);

    int i = std::floor(u);
    int j = std::floor(v);
    float du = u - i;
    float dv = v - j;

    // 2. 주변 4x4 픽셀 샘플링
    float rowResults[4];
    for (int row = 0; row < 4; ++row)
    {
        float p[4];
        for (int col = 0; col < 4; ++col) 
        {
            p[col] = getPixel(i - 1 + col, j - 1 + row);
        }
        rowResults[row] = cubicInterpolate(p, du);
    }

    // 3. 최종 수직 방향 보간
    return cubicInterpolate(rowResults, dv);
}

// 선형 보간을 사용하여 지오이드 높이 계산
double CGeoidGrid::getLinergeoidHeight(double lat, double lon)
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
    double v00 = data[r0 * width + c0];
    double v10 = data[r1 * width + c0];
    double v01 = data[r0 * width + c1];
    double v11 = data[r1 * width + c1];

    double v0 = v00 * (1 - dr) + v10 * dr;
    double v1 = v01 * (1 - dr) + v11 * dr;
    double rawVal = v0 * (1 - dc) + v1 * dc;

    return (rawVal * scale) + offset;
}