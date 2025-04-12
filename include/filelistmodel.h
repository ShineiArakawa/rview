#ifndef FILELISTMODEL_H
#define FILELISTMODEL_H

#include <fileutil.h>

#include <array>

// ######################################################################################
// FileListModelBase
// ######################################################################################
class FileListModelBase {
 private:
  static inline const size_t MAX_DIR_PATH_HISTORY_SIZE = 30;

  std::array<fs::path, MAX_DIR_PATH_HISTORY_SIZE> _dirPathHistory;
  int _cursor;

 protected:
  void setCurrentDir(const fs::path& dirPath);

 public:
  explicit FileListModelBase();
  virtual ~FileListModelBase() = default;

  fs::path getCurrentDir() const;

  void goBack();
  void goForward();
};

// ######################################################################################
// FileListModel
// ######################################################################################
class FileListModel : public FileListModelBase {
 private:
  std::vector<fs::path> getDirList() const;
  std::vector<fs::path> getOnlyFileList() const;

 public:
  FileListModel();
  ~FileListModel();

  void updateCurrentDir(const fs::path& dirPath);
  std::vector<fs::path> getFileList(bool filesOnly = false) const;

  static bool naturalCompare(const fs::path& a, const fs::path& b);
};

using FileListModel_t = std::shared_ptr<FileListModel>;

#endif  // FILELISTMODEL_H
