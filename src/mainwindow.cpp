#include <common.h>
#include <mainwindow.h>

#include <QClipboard>
#include <QEvent>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QGuiApplication>
#include <QMessageBox>
#include <QMimeData>
#include <algorithm>
#include <cctype>

#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      _ui(new Ui::MainWindow),
      _control(std::make_shared<MainControl>()),
      _resampleActionGroup(new QActionGroup(this)) {
  // ------------------------------------------------------------------------------------------
  // Set up ui
  _ui->setupUi(this);

  setWindowIcon(QIcon(":/icons/appico.ico"));
  setWindowTitle(Common::WINDOW_TITLE);
  setAcceptDrops(true);

  // Set splitter sizes
  const int currentWindowWidth = width();
  const int glwidgetWidth = static_cast<int>(currentWindowWidth * Common::DEFAULT_GLWIDGET_WIDTH_RATIO);
  QList<int> sizes;
  sizes << (currentWindowWidth - glwidgetWidth) << glwidgetWidth;
  _ui->splitter->setSizes(sizes);

  // ------------------------------------------------------------------------------------------
  // Set main controller
  _ui->fileListWidget->setMainControl(_control);

  // ------------------------------------------------------------------------------------------
  // Initialize file list
  const auto dirPath = fs::absolute(FileUtil::qStringToPath(QDir::homePath()));
  updateCurrentDir(dirPath);

  // ------------------------------------------------------------------------------------------
  // Connect signals
  connect(_ui->fileListWidget, &FileListWidget::signal_goParent, this, &MainWindow::goParent);
  connect(_ui->fileListWidget, &FileListWidget::signal_goChild, this, &MainWindow::goChild);
  connect(_ui->fileListWidget, &FileListWidget::signal_goBack, this, &MainWindow::goBack);
  connect(_ui->fileListWidget, &FileListWidget::signal_goForward, this, &MainWindow::goForward);
  connect(_ui->fileListWidget, &FileListWidget::signal_copyImageToClipboard, this, &MainWindow::copyImageToClipboard);

  // ------------------------------------------------------------------------------------------
  // Action group
  {
    // Resample action group
    _resampleActionGroup->setExclusive(true);
    _ui->actionNearest->setActionGroup(_resampleActionGroup);
    _ui->actionBilinear->setActionGroup(_resampleActionGroup);
    _ui->actionBicubic->setActionGroup(_resampleActionGroup);
    _ui->actionLanczos4->setActionGroup(_resampleActionGroup);
  }
}

MainWindow::~MainWindow() {
  delete _ui;
  delete _resampleActionGroup;
}

// ##############################################################################################################################
// Functions
// ##############################################################################################################################

void MainWindow::updateCurrentDir(const fs::path& dirPath) {
  const auto& currentDirPath = _control->getCurrentDir();

  // Check if the new directory exists and is a directory
  if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
    qDebug() << "Invalid directory path: " << FileUtil::pathToQString(dirPath);
    return;
  }

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

  const auto& localFiles = _control->getFileList();

  _ui->fileListWidget->clear();
  _ui->fileListWidget->addItem(Common::PATENT_DIR_REL_PATH);

  for (const auto& file : localFiles) {
    try {
      if (fs::is_directory(file) || fs::is_regular_file(file) || fs::is_symlink(file)) {
        const QFileInfo info(FileUtil::pathToQString(file));
        const auto& icon = fileIconProvider.icon(info);

        QListWidgetItem* item = new QListWidgetItem(icon, FileUtil::pathToQString(file.filename()));

        _ui->fileListWidget->addItem(item);
      }
    } catch (const std::filesystem::filesystem_error& e) {
      // Log the error or handle it accordingly
      qInfo() << "Error accessing file: " << e.what();
    }
  }
}

void MainWindow::updateImage(const fs::path& fileName) {
  const auto& imageData = _control->getImageData(fileName);

  if (!imageData.empty()) {
    // Set the image data to the OpenGL widget
    updateImage(imageData);

    // Set window title
    const auto& filePath = imageData.path;
    const auto& fileNameStr = FileUtil::pathToQString(filePath);
    setWindowTitle(Common::WINDOW_TITLE + " - " + fileNameStr);
  }
}

void MainWindow::updateImage(const ImageData& imageData) {
  if (imageData.empty()) {
    return;
  }

  // Set the image data to the OpenGL widget
  _ui->glwidget->updateTexture(imageData.image);
}

void MainWindow::goParent() {
  const QString currentDirName = FileUtil::pathToQString(_control->getCurrentDir().filename());

  if (_control->goParent()) {
    updateCurrentDir(_control->getCurrentDir());

    // Select the parent directory in the file list
    const auto parentItemList = _ui->fileListWidget->findItems(currentDirName, Qt::MatchExactly);
    for (const auto& item : parentItemList) {
      _ui->fileListWidget->setCurrentItem(item);
      _ui->fileListWidget->scrollToItem(item);
    }
  }
}

void MainWindow::goChild() {
  // Get the selected item
  QListWidgetItem* currentItem = _ui->fileListWidget->currentItem();
  if (currentItem == nullptr) {
    return;
  }

  // Get the file name from the selected item
  const auto& fileName = FileUtil::qStringToPath(currentItem->text());

  if (_control->goChild(fileName)) {
    updateCurrentDir(_control->getCurrentDir());
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

void MainWindow::copyImageToClipboard() {
  // Get the selected item
  QListWidgetItem* currentItem = _ui->fileListWidget->currentItem();
  if (currentItem == nullptr) {
    return;
  }

  const auto& fileName = FileUtil::qStringToPath(currentItem->text());
  const auto& currentDir = _control->getCurrentDir();
  const auto& filePath = currentDir / fileName;

  if (fs::exists(filePath) && fs::is_regular_file(filePath)) {
    // Copy the image to the clipboard

    QMimeData* mimeData = new QMimeData();

    QList<QUrl> urls;
    urls << QUrl::fromLocalFile(FileUtil::pathToQString(filePath));

    mimeData->setUrls(urls);
    mimeData->setText(FileUtil::pathToQString(filePath));
    QGuiApplication::clipboard()->setMimeData(mimeData);
    qDebug() << "File Url was copied to clipboard: " << FileUtil::pathToQString(filePath);
  }
}

void MainWindow::quit(bool needConfirm) {
  if (needConfirm) {
    const auto ret = QMessageBox::question(this,
                                           tr("Exit"),
                                           tr("Are you sure you want to exit?"),
                                           QMessageBox::Yes | QMessageBox::No,
                                           QMessageBox::No);

    if (ret == QMessageBox::No) {
      return;
    }
  }

  QApplication::exit();
}

// ##############################################################################################################################
// Event handlers

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
  if (event != nullptr && event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  }
}

void MainWindow::dropEvent(QDropEvent* event) {
  if (event != nullptr && event->mimeData()->hasUrls()) {
    const auto urls = event->mimeData()->urls();

    for (const auto& url : urls) {
      const auto filePath = FileUtil::qStringToPath(url.toLocalFile());
      const auto dirPath = filePath.parent_path();

      if (fs::exists(dirPath) && fs::is_directory(dirPath)) {
        updateCurrentDir(dirPath);

        // Focus on the dropped item
        // Select the parent directory in the file list
        const auto droppedItemName = FileUtil::pathToQString(filePath.filename());
        const auto itemList = _ui->fileListWidget->findItems(droppedItemName, Qt::MatchExactly);
        for (const auto& item : itemList) {
          _ui->fileListWidget->setCurrentItem(item);
          _ui->fileListWidget->scrollToItem(item);
        }

        break;
      }
    }
  }

  event->accept();
}

// ##############################################################################################################################
// Slot functions

void MainWindow::on_currentDirPath_returnPressed() {
  const auto dirPath = fs::absolute(FileUtil::qStringToPath(_ui->currentDirPath->text()));
  updateCurrentDir(dirPath);
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
    if (fileName == Common::PATENT_DIR_REL_PATH) {
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
      qInfo() << "File is not a directory: " << FileUtil::pathToQString(filePath);
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

  updateImage(FileUtil::qStringToPath(current->text()));
}

// ----------------------------------------------------------------------------------------------------------------------------------
// 'File' menu

void MainWindow::on_actionOpenDir_triggered() {
  const auto currentDirPath = FileUtil::pathToQString(fs::absolute(_control->getCurrentDir()));

  const auto dirPath = QFileDialog::getExistingDirectory(this,
                                                         tr("Open Directory"),
                                                         currentDirPath);

  if (!dirPath.isEmpty()) {
    updateCurrentDir(FileUtil::qStringToPath(dirPath));
  }
}

// ----------------------------------------------------------------------------------------------------------------------------------
// 'Resample' menu

void MainWindow::on_actionNearest_triggered() {
  _ui->glwidget->setShaderType(ImageShaderType::NEAREST);
  qDebug() << "Switched to Nearest shader";
}

void MainWindow::on_actionBilinear_triggered() {
  _ui->glwidget->setShaderType(ImageShaderType::BILINEAR);
  qDebug() << "Switched to Bilinear shader";
}

void MainWindow::on_actionBicubic_triggered() {
  _ui->glwidget->setShaderType(ImageShaderType::BICUBIC);
  qDebug() << "Switched to Bicubic shader";
}

void MainWindow::on_actionLanczos4_triggered() {
  _ui->glwidget->setShaderType(ImageShaderType::LANCZOS4);
  qDebug() << "Switched to Lanczos4 shader";
}
