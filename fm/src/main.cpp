/****************************************************************************
* This file is part of qtFM, a simple, fast file manager.
* Copyright (C) 2012, 2013 Michal Rost
* Copyright (C) 2010, 2011, 2012 Wittfella
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>
*
* Contact e-mail: rost.michal@gmail.com, wittfella@qtfm.org
*
****************************************************************************/

#include <QApplication>
#include "mainwindow.h"
#include "common.h"
#include "apptranslator.h"
#include "diagnosticlog.h"
#include <QSettings>
#ifdef Q_OS_MAC
#include <QStyleFactory>
#endif

#include <stdio.h>
#include <stdlib.h>

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
/**
 * Optional X11 backend for Wayland sessions. Default OFF on GNOME/etc.: native
 * Wayland can DnD to Wayland apps; forcing xcb breaks that and only helps some
 * XWayland targets (e.g. Thunar). Enable in Settings if needed.
 * Respect QT_QPA_PLATFORM / QTFM_NATIVE_WAYLAND / QTFM_FORCE_X11.
 */
static void applyPreferredDisplayBackend()
{
  if (qEnvironmentVariableIsSet("QT_QPA_PLATFORM"))
    return;
  if (qEnvironmentVariableIsSet("QTFM_NATIVE_WAYLAND"))
    return;

  if (qEnvironmentVariableIsSet("QTFM_FORCE_X11")) {
    qputenv("QT_QPA_PLATFORM", "xcb");
    return;
  }

  const bool waylandSession =
      qEnvironmentVariableIsSet("WAYLAND_DISPLAY")
      || qgetenv("XDG_SESSION_TYPE") == "wayland";
  if (!waylandSession)
    return;

  bool preferX11 = false;
  {
    QSettings settings(Common::configFile(), QSettings::IniFormat);
    preferX11 = settings.value(QStringLiteral("preferX11Backend"), false).toBool();
  }
  if (preferX11)
    qputenv("QT_QPA_PLATFORM", "xcb");
}
#endif

void msgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    if (localMsg.contains("link outline hasn't been detected!") ||
        localMsg.contains("iCCP: known incorrect sRGB profile") ||
        localMsg.contains("XDG_RUNTIME_DIR")) { return; }
    const char *prefix = "Msg";
    switch (type) {
    case QtDebugMsg: prefix = "Debug"; break;
#if QT_VERSION >= 0x050500
    case QtInfoMsg: prefix = "Info"; break;
#endif
    case QtWarningMsg: prefix = "Warning"; break;
    case QtCriticalMsg: prefix = "Critical"; break;
    case QtFatalMsg: prefix = "Fatal"; break;
    }
    fprintf(stderr, "%s: %s (%s:%u, %s)\n", prefix, localMsg.constData(), context.file, context.line, context.function);
    DiagnosticLog::appendLine(QByteArray(prefix) + ": " + localMsg);
    if (type == QtFatalMsg) {
        abort();
    }
}

/**
 * @brief main function
 * @param argc number of command line arguments
 * @param argv command line arguments
 * @return 0/1
 */

int main(int argc, char *argv[]) {

  qInstallMessageHandler(msgHandler);

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
  applyPreferredDisplayBackend();
#endif

  QApplication app(argc, argv);
#ifdef Q_OS_MAC
  if (QStyle *fusion = QStyleFactory::create(QStringLiteral("Fusion"))) {
    QApplication::setStyle(fusion);
  }
#endif
  QApplication::setOrganizationName(APP);
  QApplication::setApplicationName(APP);
  QApplication::setOrganizationDomain("eu");

  DiagnosticLog::openSession();
  qInfo("QtFM diagnostic log: %s", qPrintable(DiagnosticLog::filePath()));

  {
    QSettings settings(Common::configFile(), QSettings::IniFormat);
    AppTranslator::installForApplication(
        &app, settings.value(QStringLiteral("uiLanguage"), QStringLiteral("system")).toString());
  }

  // Create main window and execute application
  MainWindow mainWin;
  return app.exec();
}
