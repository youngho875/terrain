#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <stdexcept>

struct PGMHeader {
    std::string magicNumber;
    int width;
    int height;
    int maxVal;
};

// Function to read the PGM file and load geoid heights
std::vector<std::vector<double>> loadGeoidHeights(const std::string& pgmFilePath, PGMHeader& header) 
{
    std::ifstream file(pgmFilePath, std::ios::binary);
    if (!file.is_open()) 
    {
        throw std::runtime_error("Failed to open PGM file.");
    }

    // Read the PGM header
    std::getline(file, header.magicNumber);
    if (header.magicNumber != "P5") 
    {
        throw std::runtime_error("Invalid PGM file format. Expected P5.");
    }

    file >> header.width >> header.height;
    file >> header.maxVal;
    file.ignore(); // Ignore the remaining newline character

    // Load geoid heights into a 2D vector
    std::vector<std::vector<double>> geoidHeights(header.height, std::vector<double>(header.width));
    for (int y = 0; y < header.height; ++y)
    {
        for (int x = 0; x < header.width; ++x) 
        {
            unsigned char pixel;
            file.read(reinterpret_cast<char*>(&pixel), sizeof(pixel));
            geoidHeights[y][x] = static_cast<double>(pixel);  // Store pixel values as double
        }
    }

    file.close();
    return geoidHeights;
}

double getElevationFromGeoTIFF(const std::string& geoTiffPath, double latitude, double longitude) {
    GDALAllRegister();  // Register all GDAL drivers

    // Open the GeoTIFF file
    GDALDataset* dataset = (GDALDataset*)GDALOpen(geoTiffPath.c_str(), GA_ReadOnly);
    if (!dataset) 
    {
        throw std::runtime_error("Failed to open GeoTIFF file.");
    }

    // Get the raster band (assuming the elevation data is in the first band)
    GDALRasterBand* band = dataset->GetRasterBand(1);
    if (!band) 
    {
        GDALClose(dataset);
        throw std::runtime_error("Failed to get raster band.");
    }

    // Retrieve geotransform to convert geo-coordinates to pixel coordinates
    double geoTransform[6];
    if (dataset->GetGeoTransform(geoTransform) != CE_None) 
    {
        GDALClose(dataset);
        throw std::runtime_error("Failed to get geo-transform.");
    }

    // Convert geographic coordinates to pixel indices
    int pixelX = static_cast<int>((longitude - geoTransform[0]) / geoTransform[1]);
    int pixelY = static_cast<int>((latitude - geoTransform[3]) / geoTransform[5]);

    // Check for valid pixel coordinates
    if (pixelX < 0 || pixelY < 0 || pixelX >= band->GetXSize() || pixelY >= band->GetYSize()) 
    {
        GDALClose(dataset);
        throw std::out_of_range("Coordinates outside raster bounds.");
    }

    // Read the elevation value at the pixel location (assuming elevation is in 32-bit float format)
    float elevation;
    band->RasterIO(GF_Read, pixelX, pixelY, 1, 1, &elevation, 1, 1, GDT_Float32, 0, 0);

    // Close the dataset
    GDALClose(dataset);

    return static_cast<double>(elevation);
}

// Function to calculate the height above mean sea level
double getHeightAboveMSL(const std::string& pgmFilePath, double latitude, double longitude, double heightAboveEllipsoid) {
    PGMHeader header;
    std::vector<std::vector<double>>  geoidHeights = loadGeoidHeights(pgmFilePath, header);

    // Convert latitude and longitude to indices
    // Assumed that the PGM covers from -90 to +90 latitude and -180 to +180 longitude.
    int latIndex = static_cast<int>((latitude + 90) * (header.height / 180.0));
    int lonIndex = static_cast<int>((longitude + 180) * (header.width / 360.0));

    // Check bounds
    if (latIndex < 0 || latIndex >= header.height || lonIndex < 0 || lonIndex >= header.width) 
    {
        throw std::out_of_range("Coordinates outside geoid model bounds.");
    }

    // Get the geoid height
    double geoidHeight = geoidHeights[latIndex][lonIndex];

    // Calculate height above mean sea level
    return heightAboveEllipsoid - geoidHeight;
}

//path : C:\ProgramData\GeographicLib\geoids\egm96-5.pgm
// patth : D:\mapdata\gebco_08_rev_elev_D1_grey_geo.tif

int main() {
    std::string pgmFilePath, geoTiffPath;
    double latitude, longitude, heightAboveEllipsoid;

    std::cout << "Enter EGM96 PGM file path: ";
    std::cin >> pgmFilePath;
    //std::cout << "Enter geotiff file path: ";
    //std::cin >> geoTiffPath;
    std::cout << "Enter latitude (degrees): ";
    std::cin >> latitude;
    std::cout << "Enter longitude (degrees): ";
    std::cin >> longitude;
    std::cout << "Enter height above ellipsoid (meters): ";
    std::cin >> heightAboveEllipsoid;

    //heightAboveEllipsoid = getElevationFromGeoTIFF(geoTiffPath, latitude, longitude);

    try {
        double heightAboveMSL = getHeightAboveMSL(pgmFilePath, latitude, longitude, heightAboveEllipsoid);
        std::cout << "Height above Mean Sea Level (MSL): " << heightAboveMSL << " meters" << std::endl;
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}