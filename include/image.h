#pragma once

#include <TinyEXIF.h>
#include <fileutil.h>

#include <opencv2/opencv.hpp>

class ImagingUtil {
 public:
  static cv::Mat correctOrientation(const cv::Mat& img, const fs::path& filePath);
};

struct ImageData {
  ImageData() = default;                                                     // Default constructor
  ImageData(const cv::Mat& img, const fs::path& p) : image(img), path(p) {}  // Constructor with image and path

  cv::Mat image;  // OpenCV Mat object to hold the image data
  fs::path path;  // Path to the image file

  bool empty() const { return image.empty(); }  // Check if the image is empty
};

using ImageData_t = std::shared_ptr<ImageData>;
using ImageDataList_t = std::vector<ImageData_t>;
