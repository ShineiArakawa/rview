#pragma once

#include <fileutil.h>

#if defined(WITH_LIBEXIF)
#include <libexif/exif-data.h>
#endif

#include <opencv2/opencv.hpp>

class ImagingUtil {
 public:
  static cv::Mat correctOrientation(const cv::Mat& img, const fs::path& filePath) {
    #if defined(WITH_LIBEXIF)
    ExifData* ed = exif_data_new_from_file(filePath.c_str());

    if (!ed) {
      std::cerr << "Failed to read EXIF data\n";
      return img;
    }

    ExifEntry* entry = exif_data_get_entry(ed, EXIF_TAG_ORIENTATION);
    int orientation = 1;  // デフォルト：そのまま
    if (entry) {
      orientation = exif_get_short(entry->data, exif_data_get_byte_order(ed));
    }
    exif_data_unref(ed);

    cv::Mat result;
    switch (orientation) {
      case 1:
        result = img.clone();
        break;
      case 2:
        cv::flip(img, result, 1);  // 水平方向反転
        break;
      case 3:
        cv::rotate(img, result, cv::ROTATE_180);
        break;
      case 4:
        cv::flip(img, result, 0);  // 垂直方向反転
        break;
      case 5:
        cv::transpose(img, result);
        cv::flip(result, result, 1);
        break;
      case 6:
        cv::rotate(img, result, cv::ROTATE_90_CLOCKWISE);
        break;
      case 7:
        cv::transpose(img, result);
        cv::flip(result, result, 0);
        break;
      case 8:
        cv::rotate(img, result, cv::ROTATE_90_COUNTERCLOCKWISE);
        break;
      default:
        result = img.clone();
        break;
    }

    return result;
#else
    return img;
#endif
  }
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
