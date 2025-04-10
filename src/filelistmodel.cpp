#include <filelistmodel.h>

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
#if defined(_WIN32)
    _dirPathHistoryIndex = min(_dirPathHistoryIndex + 1, MAX_DIR_PATH_HISTORY_SIZE);
#else
    _dirPathHistoryIndex = std::min(_dirPathHistoryIndex + 1, MAX_DIR_PATH_HISTORY_SIZE);
#endif
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

bool naturalCompare(const std::filesystem::path& a, const std::filesystem::path& b) {
#ifdef _WIN32
  std::string aStr = FileUtil::wstringToString(a.wstring());
  std::string bStr = FileUtil::wstringToString(b.wstring());
#else
  std::string aStr = a.string();
  std::string bStr = b.string();
#endif

  auto ai = aStr.begin(), bi = bStr.begin();

  while (ai != aStr.end() && bi != bStr.end()) {
    if (*ai < 1 || *bi < 1) {
      return false;
    }

    if (std::isdigit(*ai) && std::isdigit(*bi)) {
      int aNum = 0, bNum = 0;

      while (ai != aStr.end() && std::isdigit(*ai)) {
        aNum = aNum * 10 + (*ai - '0');
        ++ai;
      }
      while (bi != bStr.end() && std::isdigit(*bi)) {
        bNum = bNum * 10 + (*bi - '0');
        ++bi;
      }

      if (aNum != bNum) {
        return aNum < bNum;
      }
    }
    // 数字以外の場合は、通常の文字として比較
    else if (*ai != *bi) {
      return *ai < *bi;
    } else {
      ++ai;
      ++bi;
    }
  }

  return aStr.size() < bStr.size();  // 長さで決まる場合も考慮
}

std::vector<fs::path> FileListModel::getDirList() const {
  std::vector<fs::path> dirList;
  const auto dirPath = getCurrentDir();

  for (const auto& entry : fs::directory_iterator(dirPath)) {
    const fs::path& path = entry.path();
    if (fs::is_directory(path)) {
      dirList.push_back(path);
    }
  }

  std::sort(dirList.begin(), dirList.end(), naturalCompare);

  return dirList;
}

std::vector<fs::path> FileListModel::getOnlyFileList() const {
  std::vector<fs::path> fileList;
  const auto dirPath = getCurrentDir();

  for (const auto& entry : fs::directory_iterator(dirPath)) {
    const fs::path& path = entry.path();
    if (fs::is_regular_file(path)) {
      fileList.push_back(path);
    }
  }

  std::sort(fileList.begin(), fileList.end(), naturalCompare);

  return fileList;
}

std::vector<fs::path> FileListModel::getFileList(bool filesOnly) const {
  std::vector<fs::path> fileList = getOnlyFileList();

  if (!filesOnly) {
    // Combine the directory and file lists
    std::vector<fs::path> dirList = getDirList();
    fileList.insert(fileList.begin(), dirList.begin(), dirList.end());
  }

  return fileList;
}
