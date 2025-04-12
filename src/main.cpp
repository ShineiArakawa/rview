#include <QApplication>
#include <QLocale>
#include <QSurfaceFormat>
#include <QTranslator>

#include "mainwindow.h"

int main(int argc, char* argv[]) {
  // ------------------------------------------------------------------------------------------
  // Initialize opengl context
  // Set up OpenGL context format

  QSurfaceFormat format;
  format.setMajorVersion(4);
  format.setMinorVersion(1);
  format.setSamples(4);  // 4x MSAA
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
  format.setDepthBufferSize(24);
  QSurfaceFormat::setDefaultFormat(format);

  // ------------------------------------------------------------------------------------------
  // Create app
  QApplication a(argc, argv);

  // ------------------------------------------------------------------------------------------
  // Set up the application
  QTranslator translator;
  const QStringList uiLanguages = QLocale::system().uiLanguages();
  for (const QString& locale : uiLanguages) {
    const QString baseName = "ReView_" + QLocale(locale).name();
    if (translator.load(":/i18n/" + baseName)) {
      a.installTranslator(&translator);
      break;
    }
  }

  // ------------------------------------------------------------------------------------------
  // Main window
  MainWindow w;

  if (argc > 1) {
    const auto dirPath = fs::absolute(fs::path(argv[1]));
    w.updateCurrentDir(dirPath);
  }

  w.show();

  return a.exec();
}
