#include <maincontrol.h>

#include <exiv2/exiv2.hpp>

MainControl::MainControl()
    : _fileListModel(std::make_shared<FileListModel>()) {
}

MainControl::~MainControl() = default;

// ################################################################################################################
// File list model
void MainControl::setCurrentDir(const fs::path& dirPath) {
  _fileListModel->updateCurrentDir(dirPath);
}

fs::path MainControl::getCurrentDir() const {
  return _fileListModel->getCurrentDir();
}

std::vector<fs::path> MainControl::getFileList() const {
  return _fileListModel->getFileList();
}

void MainControl::goBack() {
  _fileListModel->goBack();
}

void MainControl::goForward() {
  _fileListModel->goForward();
}

// ################################################################################################################
// Image data
ImageData MainControl::getImageData(const fs::path& filename) const {
  const auto currentDir = getCurrentDir();
  const auto filePath = currentDir / filename;

  const std::set<std::string> supportedExtensions = {
      ".png",
      ".jpg",
      ".jpeg",
      ".bmp",
      ".tiff",
      ".tif",
      ".exr",
  };

  if (!fs::exists(filePath)) {
    qInfo() << "File does not exist: " << FileUtil::pathToString(filePath);
    return ImageData();
  }

  if (!fs::is_regular_file(filePath)) {
    qInfo() << "File is not a regular file: " << FileUtil::pathToString(filePath);
    return ImageData();
  }

  std::string fileExtension = filePath.extension().string();
  std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);

  if (supportedExtensions.find(fileExtension) == supportedExtensions.end()) {
    qInfo() << "Unsupported file format: " << FileUtil::pathToString(filePath);
    return ImageData();
  }

  qInfo() << "File path: " << FileUtil::pathToString(filePath);
  cv::Mat image = cv::imread(FileUtil::pathToString(filePath), cv::IMREAD_UNCHANGED);

  if (image.empty()) {
    return ImageData();
  }

  try {
    auto exifImg = Exiv2::ImageFactory::open(FileUtil::pathToString(filePath));
    exifImg->readMetadata();
    Exiv2::ExifData& exifData = exifImg->exifData();
    int64_t orientation = exifData["Exif.Image.Orientation"].toInt64();

    switch (orientation) {
      case 6:  // 右に90度回転
        cv::rotate(image, image, cv::ROTATE_90_CLOCKWISE);
        break;
      case 8:  // 左に90度回転
        cv::rotate(image, image, cv::ROTATE_90_COUNTERCLOCKWISE);
        break;
      case 3:  // 180度回転
        cv::rotate(image, image, cv::ROTATE_180);
        break;
        // その他の値ではそのまま
    }
  } catch (...) {
    qInfo() << "Failed to read EXIF data: " << FileUtil::pathToString(filePath);
  }

  return ImageData(image, filePath);
}
