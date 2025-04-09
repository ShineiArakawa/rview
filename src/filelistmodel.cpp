#include <filelistmodel.h>
#include <naturalsort/naturalorder.h>

#include <algorithm>

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
    _dirPathHistoryIndex = min(_dirPathHistoryIndex + 1, MAX_DIR_PATH_HISTORY_SIZE);
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
  std::vector<fs::path> dirList;
  std::vector<fs::path> fileList;

  const auto dirPath = getCurrentDir();

  for (const auto& entry : fs::directory_iterator(dirPath)) {
    const fs::path& path = entry.path();
    if (fs::is_directory(path)) {
      dirList.push_back(path);
    } else {
      fileList.push_back(path);
    }
  }

  // Sort the file list by natural order
  // std::sort(dirList.begin(), dirList.end(), [](const fs::path& a, const fs::path& b) -> bool {
  //     const std::string str_a = a.filename().string();
  //     const std::string str_b = b.filename().string();
  //     return (natural_compare(str_a, str_b) == 0); });
  // std::sort(fileList.begin(), fileList.end(), [](const fs::path& a, const fs::path& b) -> bool {
  //     const std::string str_a = a.filename().string();
  //     const std::string str_b = b.filename().string();
  //     return (natural_compare(str_a, str_b) == 0); });

  // Combine the directory and file lists
  fileList.insert(fileList.begin(), dirList.begin(), dirList.end());

  return fileList;
}
