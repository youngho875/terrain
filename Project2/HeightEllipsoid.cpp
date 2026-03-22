/*
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cmath>

const int DTED_HEADER_SIZE = 24; // DTED Level 2 header size
const int GRID_SIZE = 3601; // For DTED Level 2 (3601x3601)


// Function to get height above mean sea level from a DTED Level 2 file
double getHeightFromDTED(const std::string& dtedFilePath, double latitude, double longitude) {
    std::ifstream dtedFile(dtedFilePath, std::ios::binary);
    if (!dtedFile) {
        throw std::runtime_error("Failed to open DTED file.");
    }

    // Read header information
    char header[DTED_HEADER_SIZE];
    dtedFile.read(header, DTED_HEADER_SIZE);

    // Calculate indices
    int latIndex = static_cast<int>((latitude + 90.0) * (GRID_SIZE / 180.0));
    int lonIndex = static_cast<int>((longitude + 180.0) * (GRID_SIZE / 360.0));

    if (latIndex < 0 || latIndex >= GRID_SIZE || lonIndex < 0 || lonIndex >= GRID_SIZE) {
        throw std::out_of_range("Coordinates outside DTED grid bounds.");
    }

    // Read the elevation data from the DTED file
    dtedFile.seekg(DTED_HEADER_SIZE + (latIndex * GRID_SIZE + lonIndex) * sizeof(short), std::ios::beg);
    short elevation;
    dtedFile.read(reinterpret_cast<char*>(&elevation), sizeof(elevation));

    if (dtedFile.fail()) {
        throw std::runtime_error("Failed to read elevation data from DTED.");
    }

    return static_cast<double>(elevation);
}

// Function to calculate height above the ellipsoid
double getHeightAboveEllipsoid(const std::string& dtedFilePath, double latitude, double longitude, double heightAboveGround) {
    // Get the height from the DTED data
    double elevation = getHeightFromDTED(dtedFilePath, latitude, longitude);

    // Height above the ellipsoid is elevation plus height above ground
    return elevation + heightAboveGround;
}

int main() {
    std::string dtedFilePath;
    double latitude, longitude, heightAboveGround;

    std::cout << "Enter DTED Level 2 file path: ";
    std::cin >> dtedFilePath;
    std::cout << "Enter latitude (degrees): ";
    std::cin >> latitude;
    std::cout << "Enter longitude (degrees): ";
    std::cin >> longitude;
    std::cout << "Enter height above ground (meters): ";
    std::cin >> heightAboveGround;

    try {
        double heightAboveEllipsoid = getHeightAboveEllipsoid(dtedFilePath, latitude, longitude, heightAboveGround);
        std::cout << "Height above Ellipsoid: " << heightAboveEllipsoid << " meters" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
*/

#include <iostream>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <stdexcept>

double getElevationFromGeoTIFF(const std::string& geoTiffPath, double latitude, double longitude) {
    GDALAllRegister();  // Register all GDAL drivers

    // Open the GeoTIFF file
    GDALDataset* dataset = (GDALDataset*)GDALOpen(geoTiffPath.c_str(), GA_ReadOnly);
    if (!dataset) {
        throw std::runtime_error("Failed to open GeoTIFF file.");
    }

    // Get the raster band (assuming the elevation data is in the first band)
    GDALRasterBand* band = dataset->GetRasterBand(1);
    if (!band) {
        GDALClose(dataset);
        throw std::runtime_error("Failed to get raster band.");
    }

    // Retrieve geotransform to convert geo-coordinates to pixel coordinates
    double geoTransform[6];
    if (dataset->GetGeoTransform(geoTransform) != CE_None) {
        GDALClose(dataset);
        throw std::runtime_error("Failed to get geo-transform.");
    }

    // Convert geographic coordinates to pixel indices
    int pixelX = static_cast<int>((longitude - geoTransform[0]) / geoTransform[1]);
    int pixelY = static_cast<int>((latitude - geoTransform[3]) / geoTransform[5]);

    // Check for valid pixel coordinates
    if (pixelX < 0 || pixelY < 0 || pixelX >= band->GetXSize() || pixelY >= band->GetYSize()) {
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

// Function to calculate height above the ellipsoid
double getHeightAboveEllipsoid(const std::string& geoTiffPath, double latitude, double longitude, double heightAboveGround) {
    // Get elevation from the GeoTIFF data
    double elevation = getElevationFromGeoTIFF(geoTiffPath, latitude, longitude);

    // Height above the ellipsoid is elevation plus the height above ground
    return elevation + heightAboveGround;
}

int main() {
    std::string geoTiffPath;
    double latitude, longitude, heightAboveGround;

    std::cout << "Enter GeoTIFF file path: ";
    std::cin >> geoTiffPath;
    std::cout << "Enter latitude (degrees): ";
    std::cin >> latitude;
    std::cout << "Enter longitude (degrees): ";
    std::cin >> longitude;
    std::cout << "Enter height above ground (meters): ";
    std::cin >> heightAboveGround;

    try {
        double heightAboveEllipsoid = getHeightAboveEllipsoid(geoTiffPath, latitude, longitude, heightAboveGround);
        std::cout << "Height above Ellipsoid: " << heightAboveEllipsoid << " meters" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}