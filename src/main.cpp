/**********************************************************************
 *  main.cpp
 **********************************************************************
 * Copyright (C) 2020-2026 MX Authors
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
#include <QFileInfo>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QMessageBox>
#include <QProcess>
#include <QTranslator>

#include <memory>

#include "mainwindow.h"
#include "pathconstants.h"
#include <unistd.h>

#ifndef VERSION
    #define VERSION "?.?.?.?"
#endif

int main(int argc, char *argv[])
{
    const QByteArray option = argc == 2 ? QByteArray(argv[1]) : QByteArray();
    const bool informational = option == "-h" || option == "--help" || option == "--help-all"
                               || option == "-v" || option == "--version";
    std::unique_ptr<QCoreApplication> app;
    if (informational) {
        app = std::make_unique<QCoreApplication>(argc, argv);
    } else {
        if (qEnvironmentVariableIsEmpty("DISPLAY") && qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")
            && qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
            qWarning("mx-dockmaker: no display available (DISPLAY and WAYLAND_DISPLAY are both unset); "
                    "a graphical session is required to run this program.");
            return EXIT_FAILURE;
        }

        // Set Qt platform to XCB (X11) if not already set and we're in X11 environment
        if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM") && !qEnvironmentVariableIsEmpty("DISPLAY")
            && qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
            qputenv("QT_QPA_PLATFORM", "xcb");
        }

        app = std::make_unique<QApplication>(argc, argv);
    }

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
    parser.process(*app);

    QApplication::setWindowIcon(QIcon(PathConstants::APP_ICON_PATH));

    // Root guard - prevent running as root
    if (getuid() == 0) {
        QMessageBox::critical(nullptr, QObject::tr("Error"),
                              QObject::tr("You must run this program as normal user."));
        return EXIT_FAILURE;
    }

    QString file;
    const QStringList positionalArgs = parser.positionalArguments();
    if (!positionalArgs.isEmpty() && QFileInfo::exists(positionalArgs.first())) {
        file = QFileInfo(positionalArgs.first()).absoluteFilePath();
    }
    MainWindow w(nullptr, file);
    w.show();
    return QApplication::exec();
}
