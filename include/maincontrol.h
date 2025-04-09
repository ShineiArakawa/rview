#ifndef MAINCONTROL_H
#define MAINCONTROL_H

#include <filelistmodel.h>
#include <fileutil.h>
#include <image.h>

#include <memory>
#include <vector>

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

  ImageData getImageData(const fs::path& filename) const;
};

using MainControl_t = std::shared_ptr<MainControl>;

#endif  // MAINCONTROL_H
