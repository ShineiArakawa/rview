#include <common.h>
#include <maincontrol.h>

MainControl::MainControl()
    : _fileListModel(std::make_shared<FileListModel>()),
      _imageLoader(std::make_shared<AsyncImageLoader>(Common::NUM_THREADS,
                                                      Common::NUM_PRELOADED_IMAGES)) {
}

MainControl::~MainControl() = default;

void MainControl::setCurrentDir(const fs::path& dirPath) {
  // Check if the directory exists and is a directory
  if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
    qInfo() << "Invalid directory path: " << FileUtil::pathToString(dirPath);
    return;
  }

  _fileListModel->updateCurrentDir(dirPath);

  // --------------------------------------------------------------------------------------------------------------
  // Load images
  const auto& files = _fileListModel->getFileList(true);

  // Filter out image files
  std::vector<fs::path> imageFiles;
  for (const auto& file : files) {
    std::string fileExtension = file.extension().string();
    std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);

    if (fs::exists(file) &&
        fs::is_regular_file(file) &&
        SUPPORTED_IMAGE_EXTENSIONS.find(fileExtension) != SUPPORTED_IMAGE_EXTENSIONS.end()) {
      imageFiles.push_back(file);
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

bool MainControl::goParent() {
  const auto currentDir = fs::absolute(getCurrentDir());
  if (currentDir == currentDir.root_path()) {
    // Already at the root directory
    return false;
  }

  const auto parentDir = currentDir.parent_path();
  if (!parentDir.empty() && fs::exists(parentDir)) {
    setCurrentDir(parentDir);
    return true;
  }

  return false;
}

bool MainControl::goChild(const fs::path& fileName) {
  if (fileName.empty()) {
    return false;
  }

  const auto currentDir = getCurrentDir();
  const auto filePath = currentDir / fileName;

  if (fs::exists(filePath) && fs::is_directory(filePath)) {
    setCurrentDir(filePath);
    return true;
  }

  qInfo() << "File is not a directory: " << FileUtil::pathToString(filePath);

  return false;
}

void MainControl::goBack() {
  _fileListModel->goBack();
}

void MainControl::goForward() {
  _fileListModel->goForward();
}

ImageData MainControl::getImageData(const fs::path& filename) const {
  const auto currentDir = getCurrentDir();
  const auto filePath = currentDir / filename;

  // Check if the file exists and is a regular file
  if (!fs::exists(filePath) || !fs::is_regular_file(filePath)) {
    qInfo() << "File does not exist or is not a regular file: " << FileUtil::pathToString(filePath);
    return ImageData();
  }

  // Get the image data from the image loader. This will be a blocking call until the image is loaded.
  const auto imageData = _imageLoader->getImage(filePath);

  return imageData;
}
