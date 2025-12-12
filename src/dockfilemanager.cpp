/**********************************************************************
 *  dockfilemanager.cpp
 **********************************************************************
 * Copyright (C) 2017-2025 MX Authors
 *
 * Authors: Adrian
 *          MX Linux <http://mxlinux.org>
 *
 * This file is part of mx-dockmaker.
 *
 * mx-dockmaker is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mx-dockmaker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mx-dockmaker.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include "dockfilemanager.h"
#include "dockfileparser.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>

DockFileManager::DockFileManager(QObject *parent)
    : QObject(parent)
{
}

bool DockFileManager::saveConfiguration(const DockConfiguration &configuration, const QString &filePath,
                                        bool createBackup)
{
    clearLastError();
    const QString operation = tr("Saving configuration to %1").arg(filePath);
    emit operationStarted(operation);

    // Ensure scripts directory exists
    if (!ensureScriptsDirectory()) {
        setLastError(tr("Failed to create scripts directory"));
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
    }

    // Create backup if requested
    if (createBackup && QFileInfo::exists(filePath)) {
        if (!DockFileManager::createBackup(filePath)) {
            setLastError(tr("Failed to create backup file"));
            emit operationError(operation, m_lastError);
            emit operationCompleted(operation, false);
            return false;
        }
    }

    QFile file(filePath);
    if (!file.open(QFile::Text | QFile::WriteOnly)) {
        setLastError(tr("Could not open file for writing: %1").arg(filePath));
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
    }

    QTextStream out(&file);
    out << generateDockContent(configuration);
    file.close();

    // Set executable permissions
    if (!setExecutable(filePath)) {
        setLastError(tr("Failed to set executable permissions"));
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
    }

    emit operationCompleted(operation, true);
    return true;
}

bool DockFileManager::loadConfiguration(const QString &filePath, DockConfiguration &configuration)
{
    clearLastError();
    const QString operation = tr("Loading configuration from %1").arg(filePath);
    emit operationStarted(operation);

    QFile file(filePath);
    if (!file.open(QFile::Text | QFile::ReadOnly)) {
        setLastError(tr("Could not open file for reading: %1").arg(filePath));
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
    }

    QString content = file.readAll();
    file.close();

    // Use DockFileParser to parse the content
    DockFileParser parser(this);
    if (!parser.parseContent(content, configuration)) {
        setLastError(tr("Failed to parse dock file: %1").arg(parser.getLastError()));
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
    }

    configuration.setFileName(filePath);
    emit operationCompleted(operation, true);
    return true;
}

bool DockFileManager::deleteDockFile(const QString &filePath, bool removeFromMenu)
{
    clearLastError();
    const QString operation = tr("Deleting dock file %1").arg(filePath);
    emit operationStarted(operation);

    // Remove from menu first if requested
    if (removeFromMenu && !DockFileManager::removeFromMenu(filePath)) {
        setLastError(tr("Failed to remove dock from menu"));
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
    }

    // Delete the file
    if (!QFile::remove(filePath)) {
        setLastError(tr("Failed to delete file: %1").arg(filePath));
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
    }

    emit operationCompleted(operation, true);
    return true;
}

bool DockFileManager::moveDockFile(const QString &oldFilePath, const QString &newSlitLocation)
{
    clearLastError();
    const QString operation = tr("Moving dock to location %1").arg(newSlitLocation);
    emit operationStarted(operation);

    QFile file(oldFilePath);
    if (!file.open(QFile::Text | QFile::ReadOnly)) {
        setLastError(tr("Could not open file for reading: %1").arg(oldFilePath));
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
    }

    QString content = file.readAll();
    file.close();

    // Update slit location in content
    QString newLine = "sed -i 's/^session.screen0.slit.placement:.*/session.screen0.slit.placement: " + newSlitLocation
                      + "/' $HOME/.fluxbox/init";

    QRegularExpression re(QStringLiteral("^sed -i.*"), QRegularExpression::MultilineOption);
    QString updatedContent;

    if (re.match(content).hasMatch()) {
        updatedContent = content.replace(re, newLine);
    } else {
        // Add slit location at the beginning
        updatedContent = "#set up slit location\n" + newLine + "\n" + content;
    }

    // Write updated content
    if (!file.open(QFile::Text | QFile::WriteOnly | QFile::Truncate)) {
        setLastError(tr("Could not open file for writing: %1").arg(oldFilePath));
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
    }

    QTextStream out(&file);
    out << updatedContent;
    file.close();

    // Restart wmalauncher
    QProcess::execute(QStringLiteral("pkill"), QStringList() << QStringLiteral("wmalauncher"));
    QProcess::startDetached(oldFilePath, {});

    emit operationCompleted(operation, true);
    return true;
}

bool DockFileManager::addToMenu(const QString &filePath, const QString &dockName)
{
    clearLastError();
    const QString operation = tr("Adding dock to menu");
    emit operationStarted(operation);

    const QString command = QStringLiteral("sed");
    const QStringList args
        = {QStringLiteral("-i"),
           QStringLiteral("/\\[submenu\\] (Docks)/a \\t\\t\\t[exec] (%1) {%2}").arg(dockName, filePath),
           QDir::homePath() + QStringLiteral("/.fluxbox/submenus/appearance")};

    if (!runCommandQuiet(command, args)) {
        setLastError(tr("Failed to add dock to Fluxbox menu"));
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
    }

    emit operationCompleted(operation, true);
    return true;
}

bool DockFileManager::removeFromMenu(const QString &filePath)
{
    clearLastError();
    const QString operation = tr("Removing dock from menu");
    emit operationStarted(operation);

    const QString command = QStringLiteral("sed");
    const QStringList args = {QStringLiteral("-ni"), QStringLiteral("\\|%1|!p").arg(filePath),
                              QDir::homePath() + QStringLiteral("/.fluxbox/submenus/appearance")};

    if (!runCommandQuiet(command, args)) {
        setLastError(tr("Failed to remove dock from Fluxbox menu"));
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
    }

    // Restart wmalauncher
    QProcess::execute(QStringLiteral("pkill"), QStringList() << QStringLiteral("wmalauncher"));

    emit operationCompleted(operation, true);
    return true;
}

bool DockFileManager::isInMenu(const QString &filePath)
{
    QFile menuFile(QDir::homePath() + "/.fluxbox/submenus/appearance");
    if (!menuFile.open(QFile::Text | QFile::ReadOnly)) {
        return false;
    }

    QString content = menuFile.readAll();
    menuFile.close();

    return content.contains(filePath);
}

bool DockFileManager::createBackup(const QString &filePath)
{
    clearLastError();
    const QString operation = tr("Creating backup of %1").arg(filePath);
    emit operationStarted(operation);

    QString backupPath = filePath + ".~";
    if (QFile::exists(backupPath)) {
        QFile::remove(backupPath);
    }

    if (!QFile::copy(filePath, backupPath)) {
        setLastError(tr("Failed to create backup file: %1").arg(backupPath));
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
    }

    emit operationCompleted(operation, true);
    return true;
}

bool DockFileManager::setExecutable(const QString &filePath)
{
    clearLastError();
    const QString operation = tr("Setting executable permissions on %1").arg(filePath);
    emit operationStarted(operation);

    QFile file(filePath);
    if (!file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | QFile::ReadGroup | QFile::ExeGroup
                             | QFile::ReadOther | QFile::ExeOther)) {
        setLastError(tr("Failed to set executable permissions"));
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
    }

    emit operationCompleted(operation, true);
    return true;
}

bool DockFileManager::ensureScriptsDirectory()
{
    QString scriptsDir = QDir::homePath() + "/.fluxbox/scripts";
    QDir dir;

    if (!dir.exists(scriptsDir)) {
        if (!dir.mkpath(scriptsDir)) {
            setLastError(tr("Failed to create scripts directory: %1").arg(scriptsDir));
            return false;
        }
    }

    return true;
}

QString DockFileManager::getDefaultDockDirectory()
{
    return QDir::homePath() + "/.fluxbox/scripts";
}

QString DockFileManager::getLastError() const
{
    return m_lastError;
}

QString DockFileManager::generateDockContent(const DockConfiguration &configuration)
{
    QString content;
    QTextStream out(&content);

    // Add shebang and pkill wmalauncher
    out << "#!/bin/bash\n\n";
    out << "pkill wmalauncher\n\n";

    // Add slit location setup
    QString slitLocation = configuration.getSlitLocation();
    if (!slitLocation.isEmpty()) {
        out << "#set up slit location\n";
        out << "sed -i 's/^session.screen0.slit.placement:.*/session.screen0.slit.placement: " << slitLocation
            << "/' $HOME/.fluxbox/init\n\n";
        out << "fluxbox-remote restart; sleep 1\n\n";
    }

    out << "#commands for dock launchers\n";

    // Add applications
    for (const auto &app : configuration.getApplications()) {
        if (!app.isValid()) {
            continue; // Skip incomplete entries
        }
        QString command;
        if (app.isDesktopFile()) {
            command = "--desktop-file " + escapeShellArg(app.appName);
        } else {
            const QString iconPath
                = app.customIcon.isEmpty() ? QStringLiteral("application-x-executable") : app.customIcon;
            command = "--command " + app.command + " --icon " + escapeShellArg(iconPath);
        }

        QString extraOptions = app.extraOptions;
        if (!extraOptions.isEmpty() && !extraOptions.startsWith(QLatin1Char(' '))) {
            extraOptions.prepend(QLatin1Char(' '));
        }

        QString tooltipOption;
        if (!app.tooltip.isEmpty()) {
            tooltipOption = QStringLiteral(" --tooltip-text ") + escapeShellArg(app.tooltip);
        }

        out << "wmalauncher " << command << " --background-color \"" << app.backgroundColor.name() << "\""
            << " --hover-background-color \"" << app.hoverBackground.name() << "\""
            << " --border-color \"" << app.borderColor.name() << "\""
            << " --hover-border-color \"" << app.hoverBorder.name() << "\""
            << " --window-size " << app.size.section(QStringLiteral("x"), 0, 0) << tooltipOption << extraOptions
            << "& sleep 0.1\n";
    }

    return content;
}

QString DockFileManager::escapeShellArg(const QString &arg)
{
    if (arg.isEmpty()) {
        return QStringLiteral("''");
    }

    // POSIX-safe single-quote escaping
    QString escaped = arg;
    escaped.replace(QLatin1Char('\''), QStringLiteral("'\"'\"'"));
    return QLatin1Char('\'') + escaped + QLatin1Char('\'');
}

void DockFileManager::setLastError(const QString &error)
{
    m_lastError = error;
    emit operationError(tr("File operation"), error);
}

void DockFileManager::clearLastError()
{
    m_lastError.clear();
}

bool DockFileManager::runCommandQuiet(const QString &command, const QStringList &args)
{
    QProcess proc;
    proc.start(command, args);
    proc.waitForFinished();
    return proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0;
}
