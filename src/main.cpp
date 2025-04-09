#include <QApplication>
#include <QLocale>
#include <QSurfaceFormat>
#include <QTranslator>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
  // ------------------------------------------------------------------------------------------
  // Initialize opengl context
  // Set up OpenGL context format
  // setSurfaceType(QSurface::OpenGLSurface);

  QSurfaceFormat format;
  format.setMajorVersion(4);
  format.setMinorVersion(1);
  format.setSamples(4);  // 4x MSAA
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
  format.setDepthBufferSize(24);
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
