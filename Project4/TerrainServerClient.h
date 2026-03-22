#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <curl/curl.h>
#include <zlib.h>



class TerrainServerClient
{
public:

    void fetchAndParse(const std::string& url);
    void processBinaryTerrain(const std::vector<char>& data);

    // 데이터 다운로드를 위한 콜백 함수
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::vector<char>* userp);

    // Gzip 압축 해제 함수 (zlib 사용)
    std::vector<char> decompressGzip(const std::vector<char>& compressedData);

};