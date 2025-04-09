#pragma once

#include <fileutil.h>

#include <opencv2/opencv.hpp>

struct ImageData {
  ImageData() = default;                                                     // Default constructor
  ImageData(const cv::Mat& img, const fs::path& p) : image(img), path(p) {}  // Constructor with image and path

  cv::Mat image;  // OpenCV Mat object to hold the image data
  fs::path path;  // Path to the image file

  bool empty() const { return image.empty(); }  // Check if the image is empty
};

using ImageData_t = std::shared_ptr<ImageData>;
using ImageDataList_t = std::vector<ImageData_t>;
