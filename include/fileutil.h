#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <QDir>
#include <QFileInfo>
#include <QOperatingSystemVersion>
#include <QProcess>
#include <QString>
#include <filesystem>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#elif __APPLE__
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#elif __linux__
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

class FileUtil {
 public:
  FileUtil() = delete;  // Prevent instantiation of this class

#ifdef _WIN32
  static std::string wstringToString(const std::wstring& wstr);
#endif

  static fs::path getHomeDirectory();

  static fs::path qStringToPath(const QString& qstr);
  static QString pathToQString(const fs::path& path);

  static std::string pathToString(const fs::path& path);
  static fs::path stringToPath(const std::string& str);

  static void pathToChar(const fs::path& path, char* charFilePath, std::size_t bufferSize);

  static QString absolutePath(const QString& path);

  static QString concatPath(const QString& path1, const QString& path2);
  static fs::path concatPath(const fs::path& path1, const fs::path& path2);
  static std::string concatPath(const std::string& path1, const std::string& path2);

  static bool moveToTrash(const fs::path& path);
};

#endif  // FILEUTIL_H
