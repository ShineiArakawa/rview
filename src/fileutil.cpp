#include <fileutil.h>

fs::path FileUtil::getHomeDirectory() {
  std::string homeDir;

#ifdef _WIN32
  char path[MAX_PATH];
  if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path))) {
    homeDir = path;
  }
#elif __APPLE__ || __linux__
  const char* home = getenv("HOME");
  if (home == nullptr) {
    homeDir = getpwuid(getuid())->pw_dir;
  } else {
    homeDir = home;
  }
#endif

  return fs::path(homeDir);
}

std::filesystem::path FileUtil::qStringToPath(const QString& qstr) {
#ifdef _WIN32
  return std::filesystem::path(qstr.toStdWString());
#else
  // POSIX�n�iLinux, macOS�j�̏ꍇ�FUTF-8 (std::string) ���g�p
  return std::filesystem::path(qstr.toUtf8().constData());
#endif
}

QString FileUtil::pathToQString(const fs::path& path) {
#ifdef _WIN32
  return QString::fromStdWString(path.wstring());
#else
  // POSIX�n�iLinux, macOS�j�̏ꍇ�FUTF-8 (std::string) ���g�p
  return QString::fromUtf8(path.string().c_str());
#endif
}

std::string FileUtil::pathToString(const fs::path& path) {
  return path.string();
}

fs::path FileUtil::stringToPath(const std::string& str) {
  return fs::path(str);
}

void FileUtil::pathToChar(const fs::path& path, char* charFilePath, std::size_t bufferSize) {
#ifdef _WIN32
  std::wstring wstr = path.wstring();
  std::wcstombs(charFilePath, wstr.c_str(), bufferSize);
#else
  std::string str = path.string();
  std::strncpy(charFilePath, str.c_str(), bufferSize);
#endif
}

QString FileUtil::absolutePath(const QString& path) {
  return pathToQString(fs::absolute(qStringToPath(path)));
}

QString FileUtil::concatPath(const QString& path1, const QString& path2) {
  if (path1.isEmpty()) {
    return path2;
  }

  if (path2.isEmpty()) {
    return path1;
  }

  return path1 + "/" + path2;
}

fs::path FileUtil::concatPath(const fs::path& path1, const fs::path& path2) {
  // Concatenate path1 and path2 as a string
  if (path1.empty()) {
    return path2;
  }

  if (path2.empty()) {
    return path1;
  }

  return qStringToPath(concatPath(pathToQString(path1), pathToQString(path2)));
}

std::string FileUtil::concatPath(const std::string& path1, const std::string& path2) {
  return path1 + "/" + path2;
}

bool FileUtil::moveToTrash(const fs::path& path) {
  QFileInfo fileInfo(path);

  if (!fileInfo.exists()) {
    return false;
  }

#ifdef Q_OS_WIN
  QString command = "cmd.exe";
  QStringList arguments;
  arguments << "/c"
            << "move"
            << "/y"
            << pathToQString(path)
            << "C:\\$Recycle.Bin\\";
#elif defined(Q_OS_MACOS)
  QString command = "osascript";
  QStringList arguments;
  arguments << "-e" << QString("tell application \"Finder\" to delete POSIX file \"%1\"").arg(pathToQString(path));
#elif defined(Q_OS_LINUX)
  QString command = "gio";
  QStringList arguments;
  arguments << "trash" << pathToQString(path);
#else
  return false;  // Unsupported OS
#endif

  const int returnCode = QProcess::execute(command, arguments);

  if (returnCode != 0) {
    qDebug() << "Failed to move to trash (return code: " << returnCode << "): " << pathToQString(path);
  }

  return returnCode == 0;
}
