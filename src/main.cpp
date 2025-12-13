/**********************************************************************
 *  main.cpp
 **********************************************************************
 * Copyright (C) 2020-2025 MX Authors
 *
 * Authors: Adrian
 *          MX Linux <http://mxlinux.org>
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this package. If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QMessageBox>
#include <QProcess>
#include <QTranslator>

#include "mainwindow.h"
#include "pathconstants.h"
#include <unistd.h>

#ifndef VERSION
    #define VERSION "?.?.?.?"
#endif

int main(int argc, char *argv[])
{
    // Set Qt platform to XCB (X11) if not already set and we're in X11 environment
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        if (!qEnvironmentVariableIsEmpty("DISPLAY") && qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
            qputenv("QT_QPA_PLATFORM", "xcb");
        }
    }

    QApplication a(argc, argv);
    QApplication::setWindowIcon(QIcon(PathConstants::APP_ICON_PATH));
    QApplication::setApplicationName(QStringLiteral("mx-dockmaker"));
    QApplication::setOrganizationName(QStringLiteral("MX-Linux"));
    QApplication::setApplicationVersion(VERSION);

    QTranslator qtTran;
    if (qtTran.load("qt_" + QLocale().name(), QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        QApplication::installTranslator(&qtTran);
    }

    QTranslator qtBaseTran;
    if (qtBaseTran.load("qtbase_" + QLocale().name(), QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        QApplication::installTranslator(&qtBaseTran);
    }

    QTranslator appTran;
    if (appTran.load(QApplication::applicationName() + "_" + QLocale().name(),
                     "/usr/share/" + QApplication::applicationName() + "/locale")) {
        QApplication::installTranslator(&appTran);
    }

    // Command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("MX Dockmaker - Create and manage docks for FluxBox"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", QObject::tr("Dock file to edit (optional)"), "[file]");
    parser.process(a);

    // Root guard - prevent running as root
    if (getuid() == 0) {
        QMessageBox::critical(nullptr, QObject::tr("Error"),
                              QObject::tr("You must run this program as normal user."));
        return EXIT_FAILURE;
    }

    QString file;
    const QStringList positionalArgs = parser.positionalArguments();
    if (!positionalArgs.isEmpty() && QFile::exists(positionalArgs.first())) {
        file = positionalArgs.first();
    }
    MainWindow w(nullptr, file);
    w.show();
    return QApplication::exec();
}
