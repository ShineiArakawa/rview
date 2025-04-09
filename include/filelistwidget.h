#ifndef FILELISTWIDGET_H
#define FILELISTWIDGET_H

#include <QListWidget>
#include <maincontrol.h>
#include <QMouseEvent>


class FileListWidget : public QListWidget {
  Q_OBJECT

 private:
  MainControl_t _control;

 signals:
  void signal_updateLocalFileList();

  void signal_updateRemoteFileList();

  void signal_goBack();

  void signal_goForward();

 public:
  explicit FileListWidget(QWidget* parent = nullptr);
  ~FileListWidget() = default;

  void setMainControl(MainControl_t& control);

 protected:
  void mouseReleaseEvent(QMouseEvent* event) override;
};

#endif  // FILELISTWIDGET_H
