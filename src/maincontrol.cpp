#include <maincontrol.h>

MainControl::MainControl()
    : _fileListModel(std::make_shared<FileListModel>()) {
}

MainControl::~MainControl() = default;

// ------------------------------------------------------------------
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

// ------------------------------------------------------------------