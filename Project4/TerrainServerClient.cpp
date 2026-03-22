#include "TerrainServerClient.h"


// 데이터 다운로드를 위한 콜백 함수
size_t TerrainServerClient::WriteCallback(void* contents, size_t size, size_t nmemb, std::vector<char>* userp)
{
    size_t realsize = size * nmemb;
    userp->insert(userp->end(), (char*)contents, (char*)contents + realsize);
    return realsize;
}

// Gzip 압축 해제 함수 (zlib 사용)
std::vector<char> TerrainServerClient::decompressGzip(const std::vector<char>& compressedData)
{
    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    // 16을 더하면 gzip 헤더를 자동으로 처리합니다.
    if (inflateInit2(&zs, 16 + MAX_WBITS) != Z_OK) return {};

    zs.next_in = (Bytef*)compressedData.data();
    zs.avail_in = compressedData.size();

    int ret;
    char outbuffer[32768];
    std::vector<char> outData;

    do
    {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);
        ret = inflate(&zs, Z_NO_FLUSH);
        if (outData.size() < zs.total_out)
        {
            outData.insert(outData.end(), outbuffer, outbuffer + (zs.total_out - outData.size()));
        }
    } while (ret == Z_OK);

    inflateEnd(&zs);
    return outData;
}

void TerrainServerClient::fetchAndParse(const std::string& url)
{
    CURL* curl = curl_easy_init();
    std::vector<char> downloadedData;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &downloadedData);
        // 필요시 User-Agent나 Accept 헤더 추가
        // curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            // 1. 압축 해제
            std::vector<char> rawTerrain = decompressGzip(downloadedData);

            // 2. 파싱 (기존에 작성한 파싱 로직 호출)
            processBinaryTerrain(rawTerrain);
        }
        curl_easy_cleanup(curl);
    }
}

void TerrainServerClient::processBinaryTerrain(const std::vector<char>& data)
{
    if (data.empty()) return;

    // 메모리 스트림처럼 처리
    const char* ptr = data.data();

    // 헤더 읽기 예시
    // TerrainHeader* header = reinterpret_cast<TerrainHeader*>(ptr);
    // ptr += sizeof(TerrainHeader);

    std::cout << "성공적으로 " << data.size() << " 바이트 데이터를 읽었습니다." << std::endl;
}