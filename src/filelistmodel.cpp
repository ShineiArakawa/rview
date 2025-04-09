#include <filelistmodel.h>

// -------------------------------------------------------------------------------------------------------------------
// FileListModelBase
FileListModelBase::FileListModelBase()
    : _dirPathHistory(MAX_DIR_PATH_HISTORY_SIZE),
      _dirPathHistoryIndex(0) {
}

void FileListModelBase::setCurrentDir(const fs::path& dirPath) {
  if (_dirPathHistoryIndex == 0 || _dirPathHistory[_dirPathHistoryIndex - 1] != dirPath) {
    // Clear the history after the current index
    for (size_t i = _dirPathHistoryIndex; i < _dirPathHistory.size(); i++) {
      _dirPathHistory[i] = fs::path();
    }

    _dirPathHistory[_dirPathHistoryIndex] = dirPath;
    _dirPathHistoryIndex = std::min(_dirPathHistoryIndex + 1, MAX_DIR_PATH_HISTORY_SIZE);
  }
}

fs::path FileListModelBase::getCurrentDir() const {
  if (_dirPathHistoryIndex == 0) {
    return fs::path();
  }

  return _dirPathHistory[_dirPathHistoryIndex - 1];
}

void FileListModelBase::goBack() {
  if (_dirPathHistoryIndex > 1) {
    _dirPathHistoryIndex--;
  }
}

void FileListModelBase::goForward() {
  if (_dirPathHistoryIndex < _dirPathHistory.size() && _dirPathHistory[_dirPathHistoryIndex] != fs::path()) {
    _dirPathHistoryIndex++;
  }
}

// -------------------------------------------------------------------------------------------------------------------
// FileListModel
FileListModel::FileListModel()
    : FileListModelBase() {
}

FileListModel::~FileListModel() = default;

void FileListModel::updateCurrentDir(const fs::path& dirPath) {
  setCurrentDir(dirPath);
}

std::vector<fs::path> FileListModel::getFileList() const {
  std::vector<fs::path> fileList;

  const auto dirPath = getCurrentDir();

  for (const auto& entry : fs::directory_iterator(dirPath)) {
    fileList.push_back(entry.path());
  }

  return fileList;
}