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
  std::string aStr = FileUtil::wstringToString(a.wstring());
  std::string bStr = FileUtil::wstringToString(b.wstring());

  auto itA = aStr.begin(), itB = bStr.begin();
  while (itA != aStr.end() && itB != bStr.end()) {
    // Check range
    if (*itA < 1 || *itA > 255 || *itB < 1 || *itB > 255) {
      return false;  // Invalid character range
    }

    if (std::isdigit(*itA) && std::isdigit(*itB)) {
      std::string numA, numB;
      while (itA != aStr.end() && std::isdigit(*itA)) {
        numA.push_back(*itA);
        ++itA;
      }
      while (itB != bStr.end() && std::isdigit(*itB)) {
        numB.push_back(*itB);
        ++itB;
      }
      // 数字部分の長さが異なる場合、桁数が少ないほうが小さいと判断
      if (numA.size() != numB.size()) {
        return numA.size() < numB.size();
      }
      // 桁数が同じ場合、文字列比較で決定
      if (numA != numB) {
        return numA < numB;
      }
      // 数字部分が等しければ、ループを継続して次の文字へ
    } else {
      // 文字が数字以外の場合は、大文字・小文字を無視して比較
      char ca = std::tolower(*itA);
      char cb = std::tolower(*itB);
      if (ca != cb) {
        return ca < cb;
      }
      ++itA;
      ++itB;
    }
  }
  // いずれかの文字列が終わった場合、残りの文字列の長さで比較
  return aStr.size() < bStr.size();
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

  std::sort(dirList.begin(), dirList.end(), naturalCompare);
  std::sort(fileList.begin(), fileList.end(), naturalCompare);

  // Combine the directory and file lists
  fileList.insert(fileList.begin(), dirList.begin(), dirList.end());

  return fileList;
}
