#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <fileutil.h>
#include <glwidget.h>
#include <maincontrol.h>

#include <QActionGroup>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QMenuBar>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

 private:
  Ui::MainWindow *_ui;
  MainControl_t _control;
  QActionGroup *_resampleActionGroup;

  void updateFileList();
  void updateImage(const fs::path &fileName);
  void updateImage(const ImageData &imageData);

  void goParent();
  void goChild();
  void goBack();
  void goForward();
  void copyImageToClipboard();

  void quit(bool needConfirm = false);

 private slots:
  void on_currentDirPath_returnPressed();
  void on_fileListWidget_itemDoubleClicked(QListWidgetItem *item);
  void on_fileListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

  // Menu bar
  void on_actionNearest_triggered();
  void on_actionOpenDir_triggered();
  void on_actionBilinear_triggered();
  void on_actionBicubic_triggered();
  void on_actionLanczos4_triggered();

 protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent* event) override;

 public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  void updateCurrentDir(const fs::path &dirPath);
};

#endif  // MAINWINDOW_H
