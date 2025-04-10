#ifndef FILELISTMODEL_H
#define FILELISTMODEL_H

#include <fileutil.h>

// ######################################################################################
// FileListModelBase
// ######################################################################################
class FileListModelBase {
 private:
  std::vector<fs::path> _dirPathHistory;
  size_t _dirPathHistoryIndex;

 protected:
  void setCurrentDir(const fs::path& dirPath);

 public:
  static inline const size_t MAX_DIR_PATH_HISTORY_SIZE = 30;

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
};

using FileListModel_t = std::shared_ptr<FileListModel>;

#endif  // FILELISTMODEL_H
