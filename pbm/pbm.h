#include <vector>
#include <cstdint>
#include <string>

class BinaryImage
{
public:
    int W;
    int H;
    std::vector<uint8_t> ImageData;

public:
    BinaryImage() = default;
    ~BinaryImage() = default;

    bool ReadFromPBM(const std::string& filename);
};

class Image
{
public:
    int W;
    int H;
    std::vector<uint8_t> ImageData;

public:
    Image() = default;
    ~Image() = default;
    
};

extern Image BinaryImageToImage(const BinaryImage& bimg);


