#include <mainwindow.h>

#include <QFileIconProvider>
#include <algorithm>

#include "./ui_mainwindow.h"

bool naturalSortComparator(const fs::path& a, const fs::path& b) {
  auto aStr = a.filename().string();
  auto bStr = b.filename().string();

  auto ai = aStr.begin(), bi = bStr.begin();

  while (ai != aStr.end() && bi != bStr.end()) {
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

    else if (*ai != *bi) {
      return *ai < *bi;
    } else {
      ++ai;
      ++bi;
    }
  }

  return aStr.size() < bStr.size();
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      _ui(new Ui::MainWindow),
      _control(std::make_shared<MainControl>()) {
  _ui->setupUi(this);

  // ------------------------------------------------------------------------------------------
  // Set main controller
  _ui->fileListWidget->setMainControl(_control);

  // ------------------------------------------------------------------------------------------
  // Initialize file list
  const auto dirPath = fs::absolute(FileUtil::qStringToPath(QDir::homePath()));
  updateCurrentDir(dirPath);

  // ------------------------------------------------------------------------------------------
  // Connect signals
  connect(_ui->fileListWidget, &FileListWidget::signal_goBack, this, &MainWindow::goBack);

  connect(_ui->fileListWidget, &FileListWidget::signal_goForward, this, &MainWindow::goForward);
}

MainWindow::~MainWindow() {
  delete _ui;
}

void MainWindow::updateCurrentDir(const fs::path& dirPath) {
  const auto& currentDirPath = _control->getCurrentDir();

  try {
    _control->setCurrentDir(dirPath);
    updateFileList();
    _ui->currentDirPath->setText(FileUtil::pathToQString(dirPath));
  } catch (const std::filesystem::filesystem_error& e) {
    // Log the error or handle it accordingly
    std::cerr << "Error accessing file: " << e.what() << std::endl;
    _control->setCurrentDir(currentDirPath);
  }
}

void MainWindow::updateFileList() {
  QFileIconProvider fileIconProvider;

  auto localFiles = _control->getFileList();

  // Sort the file list by natural order
  std::sort(localFiles.begin(), localFiles.end(), naturalSortComparator);

  _ui->fileListWidget->clear();

  _ui->fileListWidget->addItem(PATENT_DIR_REL_PATH);

  for (const auto& file : localFiles) {
    try {
      if (fs::is_directory(file) || fs::is_regular_file(file) || fs::is_symlink(file)) {
        const QFileInfo info(FileUtil::pathToQString(file));
        const auto icon = fileIconProvider.icon(info);

        QListWidgetItem* item = new QListWidgetItem(icon, FileUtil::pathToQString(file.filename()));

        _ui->fileListWidget->addItem(item);
      }
    } catch (const std::filesystem::filesystem_error& e) {
      // Log the error or handle it accordingly
      std::cerr << "Error accessing file: " << e.what() << std::endl;
    }
  }
}

void MainWindow::on_currentDirPath_returnPressed() {
  const auto dirPath = fs::absolute(FileUtil::qStringToPath(_ui->currentDirPath->text()));

  if (fs::exists(dirPath) && fs::is_directory(dirPath)) {
    updateCurrentDir(dirPath);
  } else {
    // Log the error or handle it accordingly
    std::cerr << "Invalid directory path: " << FileUtil::pathToString(dirPath) << std::endl;
  }
}

void MainWindow::on_fileListWidget_itemDoubleClicked(QListWidgetItem* item) {
  if (item == nullptr) {
    return;
  }

  const auto fileName = item->text();
  const auto currentDir = _control->getCurrentDir();
  const auto filePath = currentDir / FileUtil::qStringToPath(fileName);

  fs::path newDirPath;

  try {
    if (fileName == PATENT_DIR_REL_PATH) {
      // Go to the parent directory
      const auto parentDir = _control->getCurrentDir().parent_path();

      if (!parentDir.empty() && fs::exists(parentDir)) {
        newDirPath = parentDir;
      }
    } else if (fs::is_symlink(filePath)) {
      // Resolve the symlink
      const auto targetPath = fs::read_symlink(filePath);

      if (fs::exists(targetPath) && fs::is_directory(targetPath)) {
        newDirPath = targetPath;
      }
    } else if (fs::exists(filePath) && fs::is_directory(filePath)) {
      // Open the directory
      newDirPath = filePath;
    } else {
      std::cout << "File is not a directory: " << FileUtil::pathToString(filePath) << std::endl;
    }
  } catch (const std::filesystem::filesystem_error& e) {
    // Log the error or handle it accordingly
    std::cerr << "Error accessing file: " << e.what() << std::endl;
  }

  if (!newDirPath.empty()) {
    updateCurrentDir(newDirPath);
  }
}

void MainWindow::goBack() {
  _control->goBack();
  updateCurrentDir(_control->getCurrentDir());
}


void MainWindow::goForward() {
  _control->goForward();
  updateCurrentDir(_control->getCurrentDir());
}
