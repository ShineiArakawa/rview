#ifndef FILELISTWIDGET_H
#define FILELISTWIDGET_H

#include <maincontrol.h>

#include <QKeyEvent>
#include <QListWidget>
#include <QMouseEvent>

class FileListWidget : public QListWidget {
  Q_OBJECT

 private:
  MainControl_t _control;

 signals:
  void signal_updateLocalFileList();
  void signal_updateRemoteFileList();
  void signal_goParent();
  void signal_goChild();
  void signal_goBack();
  void signal_goForward();

 protected:
  void keyPressEvent(QKeyEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;

 public:
  explicit FileListWidget(QWidget* parent = nullptr);
  ~FileListWidget() = default;

  void setMainControl(MainControl_t& control);
};

#endif  // FILELISTWIDGET_H
