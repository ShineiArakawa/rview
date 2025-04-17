#include <image.h>

#include <fstream>

cv::Mat ImagingUtil::correctOrientation(const cv::Mat& img, const fs::path& filePath) {
  std::ifstream ifs(FileUtil::pathToString(filePath), std::ifstream::binary);

  if (!ifs) {
    std::cerr << "Error opening file: " << FileUtil::pathToString(filePath) << std::endl;
    return img;  // Return the original image if the file cannot be opened
  }

  // parse image EXIF and XMP metadata
  TinyEXIF::EXIFInfo imageEXIF(ifs);
  if (!imageEXIF.Fields) {
    std::cerr << "No EXIF data found in file: " << FileUtil::pathToString(filePath) << std::endl;
    return img;  // Return the original image if no EXIF data is found
  }

  // Get the orientation from EXIF data
  // uint16_t Orientation;               // Image orientation, start of data corresponds to
  //                                     // 0: unspecified in EXIF data
  //                                     // 1: upper left of image
  //                                     // 3: lower right of image
  //                                     // 6: upper right of image
  //                                     // 8: lower left of image
  //                                     // 9: undefined

  if (imageEXIF.Orientation == 3) {
    cv::Mat rotatedImg;
    cv::rotate(img, rotatedImg, cv::ROTATE_180);
    return rotatedImg;
  } else if (imageEXIF.Orientation == 6) {
    cv::Mat rotatedImg;
    cv::rotate(img, rotatedImg, cv::ROTATE_90_CLOCKWISE);
    return rotatedImg;
  } else if (imageEXIF.Orientation == 8) {
    cv::Mat rotatedImg;
    cv::rotate(img, rotatedImg, cv::ROTATE_90_COUNTERCLOCKWISE);
    return rotatedImg;
  }

  return img;  // Return the original image if no rotation is needed
}