#ifndef MAINCONTROL_H
#define MAINCONTROL_H

#include <memory>
#include <fileutil.h>
#include <vector>
#include <filelistmodel.h>

class MainControl {
 private:
  FileListModel_t _fileListModel;

 public:
  MainControl();
  ~MainControl();

  void setCurrentDir(const fs::path& dirPath);
  fs::path getCurrentDir() const;
  std::vector<fs::path> getFileList() const;
  void goBack();
  void goForward();
};

using MainControl_t = std::shared_ptr<MainControl>;

#endif  // MAINCONTROL_H
