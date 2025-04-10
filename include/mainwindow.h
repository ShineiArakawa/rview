#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <fileutil.h>
#include <glwidget.h>
#include <maincontrol.h>

#include <QListWidgetItem>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

 private:
  static inline const QString WINDOW_TITLE = "RView";
  static inline const QString PATENT_DIR_REL_PATH = "..";

 public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  void updateCurrentDir(const fs::path &dirPath);

 private slots:
  void on_currentDirPath_returnPressed();
  void on_fileListWidget_itemDoubleClicked(QListWidgetItem *item);
  void on_fileListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

 private:
  Ui::MainWindow *_ui;
  MainControl_t _control;

  void updateFileList();
  void goBack();
  void goForward();
  void updateImage(const ImageData &imageData);
};

#endif  // MAINWINDOW_H
