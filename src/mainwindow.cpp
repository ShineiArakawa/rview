#include <mainwindow.h>

#include <QFileIconProvider>
#include <algorithm>
#include <cctype>

#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      _ui(new Ui::MainWindow),
      _control(std::make_shared<MainControl>()) {
  // ------------------------------------------------------------------------------------------
  // Set up ui
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
      qInfo() << "Error accessing file: " << e.what();
    }
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

void MainWindow::updateImage(const ImageData& imageData) {
  if (imageData.empty()) {
    return;
  }

  qDebug() << "Current item changed:" << imageData.path;
  _ui->glwidget->updateTexture(imageData.image);
}

// ##############################################################################################################################
// Slot functions
// ##############################################################################################################################
void MainWindow::on_currentDirPath_returnPressed() {
  const auto dirPath = fs::absolute(FileUtil::qStringToPath(_ui->currentDirPath->text()));

  if (fs::exists(dirPath) && fs::is_directory(dirPath)) {
    updateCurrentDir(dirPath);
  } else {
    // Log the error or handle it accordingly
    qInfo() << "Invalid directory path: " << FileUtil::pathToString(dirPath);
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
      qInfo() << "File is not a directory: " << FileUtil::pathToString(filePath);
    }
  } catch (const std::filesystem::filesystem_error& e) {
    // Log the error or handle it accordingly
    qInfo() << "Error accessing file: " << e.what();
  }

  if (!newDirPath.empty()) {
    updateCurrentDir(newDirPath);
  }
}

void MainWindow::on_fileListWidget_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous) {
  if (current == nullptr) {
    return;
  }

  const auto fileName = FileUtil::qStringToPath(current->text());
  const auto imageData = _control->getImageData(fileName);

  updateImage(imageData);
}
