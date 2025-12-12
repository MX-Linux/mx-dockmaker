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
#include <QThread>
#include <QTimer>

// Fluxbox directory constants
static const QString FLUXBOX_SCRIPTS_DIR = QStringLiteral("/.fluxbox/scripts");
static const QString FLUXBOX_SUBMENUS_DIR = QStringLiteral("/.fluxbox/submenus/appearance");

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

    // Create backup if requested (createBackup handles non-existent files gracefully)
    if (createBackup && !DockFileManager::createBackup(filePath)) {
        setLastError(tr("Failed to create backup file"));
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
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

    // Restart wmalauncher - kill and wait for process to terminate before starting new instance
    killAndWaitForProcess(QStringLiteral("wmalauncher"));
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
    const QString escapedDockName = escapeSedArg(dockName);
    const QString escapedFilePath = escapeSedArg(filePath);
    const QStringList args
        = {QStringLiteral("-i"),
           QStringLiteral("/\\[submenu\\] (Docks)/a \\t\\t\\t[exec] (%1) {%2}").arg(escapedDockName, escapedFilePath),
           QDir::homePath() + FLUXBOX_SUBMENUS_DIR};

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
    const QString escapedFilePath = escapeSedArg(filePath);
    const QStringList args = {QStringLiteral("-ni"), QStringLiteral("\\|%1|!p").arg(escapedFilePath),
                              QDir::homePath() + FLUXBOX_SUBMENUS_DIR};

    if (!runCommandQuiet(command, args)) {
        setLastError(tr("Failed to remove dock from Fluxbox menu"));
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
    }

    // Kill wmalauncher to reload menu - wait for process to terminate
    killAndWaitForProcess(QStringLiteral("wmalauncher"));

    emit operationCompleted(operation, true);
    return true;
}

bool DockFileManager::isInMenu(const QString &filePath) const
{
    QFile menuFile(QDir::homePath() + FLUXBOX_SUBMENUS_DIR);
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

    // If source file doesn't exist, there's nothing to backup - return success
    if (!QFile::exists(filePath)) {
        emit operationCompleted(operation, true);
        return true;
    }

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
    QString scriptsDir = QDir::homePath() + FLUXBOX_SCRIPTS_DIR;
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
    return QDir::homePath() + FLUXBOX_SCRIPTS_DIR;
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

QString DockFileManager::escapeSedArg(const QString &arg)
{
    if (arg.isEmpty()) {
        return QString();
    }

    // Escape sed metacharacters: / \ & | ! $ ^ * ? . [ ] { } ( ) + =
    QString escaped = arg;
    escaped.replace(QRegularExpression(QStringLiteral("([/\\\\&|!$^*?.\\[\\]{}\\(\\)\\+=])")), QStringLiteral("\\\\1"));
    return escaped;
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

void DockFileManager::killAndWaitForProcess(const QString &processName)
{
    // Use pkill to terminate the process
    QProcess pkill;
    pkill.start(QStringLiteral("pkill"), {processName});
    pkill.waitForFinished(1000); // Wait up to 1 second for pkill to complete

    // Give the process time to actually terminate
    // Use pgrep to check if process still exists
    constexpr int maxAttempts = 10;
    constexpr int sleepMs = 100; // 100ms between checks
    for (int i = 0; i < maxAttempts; ++i) {
        QProcess pgrep;
        pgrep.start(QStringLiteral("pgrep"), {QStringLiteral("-x"), processName});
        pgrep.waitForFinished(500);

        // If pgrep returns non-zero, the process doesn't exist anymore
        if (pgrep.exitCode() != 0) {
            return; // Process terminated successfully
        }

        // Process still running, wait a bit
        QThread::msleep(sleepMs);
    }

    // If we get here, process is still running - try forceful kill
    QProcess::execute(QStringLiteral("pkill"), {QStringLiteral("-9"), processName});
    QThread::msleep(200); // Give it time to die
}
