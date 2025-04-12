#include <common.h>
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

void FileListWidget::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Left) {
    // Go to the parent directory
    emit signal_goParent();
  } else if (event->key() == Qt::Key_Right) {
    // Go to the child directory

    QListWidgetItem* item = currentItem();
    if (item != nullptr && item->text() != Common::PATENT_DIR_REL_PATH) {
      emit signal_goChild();
    }
  } else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
    QListWidgetItem* item = currentItem();
    if (item != nullptr) {
      if (item->text() == Common::PATENT_DIR_REL_PATH) {
        emit signal_goParent();
      } else {
        emit signal_goChild();
      }
    }
  }

  QListWidget::keyPressEvent(event);
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

  QListWidget::mouseReleaseEvent(event);
}
