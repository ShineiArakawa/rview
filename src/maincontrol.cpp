#include <maincontrol.h>

#include <exiv2/exiv2.hpp>

MainControl::MainControl()
    : _fileListModel(std::make_shared<FileListModel>()),
      _imageLoader(std::make_shared<AsyncImageLoader>()) {
}

MainControl::~MainControl() = default;

// ################################################################################################################
// File list model
void MainControl::setCurrentDir(const fs::path& dirPath) {
  _fileListModel->updateCurrentDir(dirPath);

  // --------------------------------------------------------------------------------------------------------------
  // Load images
  const auto& files = _fileListModel->getFileList(true);

  // Filter out image files
  std::vector<fs::path> imageFiles;
  for (const auto& file : files) {
    std::string fileExtension = file.extension().string();
    std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);

    if (SUPPORTED_IMAGE_EXTENSIONS.find(fileExtension) != SUPPORTED_IMAGE_EXTENSIONS.end()) {
      imageFiles.push_back(file);

      if (imageFiles.size() >= NUM_IMAGES_TO_LOAD) {
        break;  // Enough images loaded, exit the loop
      }
    }
  }

  // Load images asynchronously. This will not block the UI thread.
  // The images will be loaded in the background and can be accessed later using getImageData.
  if (!imageFiles.empty()) {
    _imageLoader->loadImages(imageFiles);
  }
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

  // Get the image data from the image loader. This will be a blocking call until the image is loaded.
  const auto imageData = _imageLoader->getImage(filePath);

  if (imageData.empty()) {
    qInfo() << "Image data is empty for file: " << FileUtil::pathToString(filePath);
  } else {
    qInfo() << "Image data loaded for file: " << FileUtil::pathToString(filePath);
  }

  return imageData;
}
