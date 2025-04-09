#include <QApplication>
#include <QLocale>
#include <QSurfaceFormat>
#include <QTranslator>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
  // OpenGLバージョン指定
  QSurfaceFormat format;
  format.setVersion(4, 1);  // OpenGL 4.1
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setSamples(4);  // 4x MSAA
  QSurfaceFormat::setDefaultFormat(format);

  QApplication a(argc, argv);

  QTranslator translator;
  const QStringList uiLanguages = QLocale::system().uiLanguages();
  for (const QString &locale : uiLanguages) {
    const QString baseName = "ReView_" + QLocale(locale).name();
    if (translator.load(":/i18n/" + baseName)) {
      a.installTranslator(&translator);
      break;
    }
  }

  MainWindow w;
  w.show();

  return a.exec();
}
