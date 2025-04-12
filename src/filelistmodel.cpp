#include <filelistmodel.h>

#include <algorithm>

// -------------------------------------------------------------------------------------------------------------------
// FileListModelBase
FileListModelBase::FileListModelBase()
    : _dirPathHistory(),
      _cursor(-1) {
  for (size_t i = 0; i < _dirPathHistory.size(); ++i) {
    _dirPathHistory[i] = fs::path();
  }
}

void FileListModelBase::setCurrentDir(const fs::path& dirPath) {
  if (_cursor >= 0 && _dirPathHistory[_cursor] == dirPath) {
    return;  // No change in the current directory
  }

  if (_cursor < static_cast<int>(_dirPathHistory.size()) - 1) {
    // Re-initialize the history with empty paths
    if (_dirPathHistory[_cursor + 1] != fs::path()) {
      std::fill(_dirPathHistory.begin() + _cursor + 1, _dirPathHistory.end(), fs::path());
    }

    _cursor++;
  } else {
    // If the history is full, remove the oldest entry
    std::rotate(_dirPathHistory.begin(), _dirPathHistory.begin() + 1, _dirPathHistory.end());
  }

  _dirPathHistory[_cursor] = dirPath;  // Set the current directory path
}

fs::path FileListModelBase::getCurrentDir() const {
  if (_cursor < 0) {
    return fs::path();
  }

  return _dirPathHistory[_cursor];
}

void FileListModelBase::goBack() {
  if (_cursor > 0) {  // Check if the previous index is within bounds
    _cursor--;
  }
}

void FileListModelBase::goForward() {
  if (_cursor < static_cast<int>(_dirPathHistory.size()) - 1  // Check if the next index is within bounds
      && _dirPathHistory[_cursor + 1] != fs::path()           // Check if the next path is valid
  ) {
    _cursor++;
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

bool FileListModel::naturalCompare(const fs::path& a, const fs::path& b) {
#ifdef _WIN32
  const std::string aStr = FileUtil::wstringToString(a.wstring());
  const std::string bStr = FileUtil::wstringToString(b.wstring());
#else
  const std::string aStr = a.string();
  const std::string bStr = b.string();
#endif

  auto ai = aStr.begin(), bi = bStr.begin();

  while (ai != aStr.end() && bi != bStr.end()) {
#if defined(_WIN32)
    if (*ai < 1 || *bi < 1) {
      return false;
    }
#endif

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

  std::sort(dirList.begin(), dirList.end(), FileListModel::naturalCompare);

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

  std::sort(fileList.begin(), fileList.end(), FileListModel::naturalCompare);

  return fileList;
}

std::vector<fs::path> FileListModel::getFileList(bool filesOnly) const {
  std::vector<fs::path> fileList = getOnlyFileList();

  if (!filesOnly) {
    // Combine the directory and file lists
    const std::vector<fs::path>& dirList = getDirList();
    fileList.insert(fileList.begin(), dirList.begin(), dirList.end());
  }

  return fileList;
}
