#include <maincontrol.h>

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

  const std::set<fs::path> supportedExtensions = {
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

  if (supportedExtensions.find(filePath.extension()) == supportedExtensions.end()) {
    qInfo() << "Unsupported file format: " << FileUtil::pathToString(filePath);
    return ImageData();
  }

  cv::Mat image = cv::imread(FileUtil::pathToString(filePath), cv::IMREAD_UNCHANGED);

  if (image.empty()) {
    return ImageData();
  }

  return ImageData(image, filePath);
}