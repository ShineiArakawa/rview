#include <filelistwidget.h>

#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>

FileListWidget::FileListWidget(QWidget* parent)
    : QListWidget(parent),
      _control(nullptr) {
  // Set context menu policy
  setContextMenuPolicy(Qt::DefaultContextMenu);
}

void FileListWidget::setMainControl(MainControl_t& control) {
  _control = control;
}

void FileListWidget::mouseReleaseEvent(QMouseEvent* event) {
  if (event->button() == Qt::XButton1) {
    // Back button
    emit signal_goBack();
  } else if (event->button() == Qt::XButton2) {
    // Forward button
    emit signal_goForward();
  } else {
    QListWidget::mouseReleaseEvent(event);
  }
}
