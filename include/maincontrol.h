#ifndef MAINCONTROL_H
#define MAINCONTROL_H

#include <filelistmodel.h>
#include <fileutil.h>
#include <image.h>
#include <imageloader.h>

#include <cctype>
#include <memory>
#include <vector>

class MainControl {
 private:
  FileListModel_t _fileListModel;
  AsyncImageLoader_t _imageLoader;

 public:
  inline static const uint32_t NUM_IMAGES_TO_LOAD = 50;
  inline static const std::set<std::string> SUPPORTED_IMAGE_EXTENSIONS = {
      ".png",
      ".jpg",
      ".jpeg",
      ".bmp",
      ".tiff",
      ".tif",
      ".exr",
  };

  MainControl();
  ~MainControl();

  void setCurrentDir(const fs::path& dirPath);
  fs::path getCurrentDir() const;
  std::vector<fs::path> getFileList() const;

  bool goParent();
  bool goChild(const fs::path& fileName);
  void goBack();
  void goForward();

  ImageData getImageData(const fs::path& filename) const;
};

using MainControl_t = std::shared_ptr<MainControl>;

#endif  // MAINCONTROL_H
