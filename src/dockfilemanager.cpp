/**********************************************************************
 *  dockfilemanager.cpp
 **********************************************************************
 * Copyright (C) 2017-2026 MX Authors
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
#include "pathconstants.h"
#include "shellarguments.h"

#include <filesystem>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QThread>
#include <QTimer>

static bool writeMenu(const QString &path, const QByteArray &content)
{
    QSaveFile menu(path);
    return menu.open(QIODevice::WriteOnly) && menu.write(content) == content.size() && menu.commit();
}

DockFileManager::DockFileManager(QObject *parent)
    : QObject(parent)
{
    m_processPool.setMaxThreadCount(1);
}

bool DockFileManager::saveConfiguration(const DockConfiguration &configuration, const QString &filePath,
                                        bool createBackup)
{
    clearLastError();
    const QString operation = tr("Saving configuration to %1").arg(filePath);
    emit operationStarted(operation);

    // Ensure scripts directory exists
    if (!ensureScriptsDirectory()) {
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
    }

    const QString content = generateDockContent(configuration);
    if (content.isNull()) {
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
    }

    // Create backup if requested (createBackup handles non-existent files gracefully)
    if (createBackup && !DockFileManager::createBackup(filePath)) {
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
    }

    if (!writeDockFile(filePath, content, true)) {
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
    if (file.error() != QFileDevice::NoError) {
        setLastError(file.errorString());
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
    }
    file.close();

    // Update slit location in content
    QString newLine = "sed -i 's/^session.screen0.slit.placement:.*/session.screen0.slit.placement: " + newSlitLocation
                      + "/' " + PathConstants::FLUXBOX_INIT_FILE;

    QRegularExpression re(QStringLiteral("^sed -i.*"), QRegularExpression::MultilineOption);
    QString updatedContent;

    if (re.match(content).hasMatch()) {
        updatedContent = content.replace(re, newLine);
    } else {
        // Add slit location at the beginning
        updatedContent = "#set up slit location\n" + newLine + "\n" + content;
    }

    if (!writeDockFile(oldFilePath, updatedContent, false)) {
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
    }

    // Restart wmalauncher without blocking the GUI while waiting for it to terminate
    restartDockAsync(QStringLiteral("wmalauncher"), oldFilePath);

    emit operationCompleted(operation, true);
    return true;
}

bool DockFileManager::addToMenu(const QString &filePath, const QString &dockName)
{
    clearLastError();
    const QString operation = tr("Adding dock to menu");
    emit operationStarted(operation);

    const QString path = QFileInfo(filePath).absoluteFilePath();
    const QString quoted = ShellArguments::quote(path).replace(QLatin1String("}"), QLatin1String("\\}"));
    QString label = dockName;
    label.replace(QLatin1String(")"), QLatin1String("\\)"));
    // A trailing backslash would escape the label's closing parenthesis.
    if (label.endsWith(QLatin1Char('\\'))) {
        label += QLatin1Char(' ');
    }
    QFile menu(QDir::homePath() + PathConstants::FLUXBOX_SUBMENUS_FILE);
    bool success = !path.contains(QLatin1Char('\n')) && !label.contains(QLatin1Char('\n')) && menu.open(QIODevice::ReadOnly);
    if (success) {
        const QByteArray original = menu.readAll();
        success = menu.error() == QFileDevice::NoError;
        menu.close();
        auto lines = original.split('\n');
        static const QRegularExpression docksHeading(QStringLiteral(R"(^\s*\[submenu\]\s*\(Docks\))"),
                                                    QRegularExpression::CaseInsensitiveOption);
        qsizetype heading = -1;
        bool found = false;
        for (qsizetype i = 0; i < lines.size(); ++i) {
            QString line = QString::fromUtf8(lines.at(i));
            if (heading < 0 && docksHeading.match(line).hasMatch()) {
                heading = i;
            }
            QRegularExpressionMatch fields;
            if (DockFileParser::menuEntryName(line, path, &fields).isEmpty()) {
                continue;
            }
            found = true;
            // Keep portable, working stock commands. Repair old unquoted absolute paths in place.
            static const QRegularExpression homePath(QStringLiteral(R"(^~/[a-zA-Z0-9_./-]+$)"));
            if (!homePath.match(fields.captured(2)).hasMatch()) {
                line.replace(fields.capturedStart(2), fields.capturedLength(2), quoted);
            }
            if (line.startsWith(QLatin1String("t\t\t"))) {
                line[0] = QLatin1Char('\t');
            }
            lines[i] = line.toUtf8();
        }
        if (!found && heading < 0) {
            success = false;
        } else if (!found) {
            QByteArray entry = QStringLiteral("\t\t\t[exec] (%1) {%2}").arg(label, quoted).toUtf8();
            if (lines.at(heading).endsWith('\r')) {
                entry.append('\r');
            }
            lines.insert(heading + 1, entry);
        }
        const QByteArray updated = lines.join('\n');
        success = success && (updated == original || writeMenu(menu.fileName(), updated));
    }
    if (!success) {
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

    QFile menu(QDir::homePath() + PathConstants::FLUXBOX_SUBMENUS_FILE);
    bool success = menu.open(QIODevice::ReadOnly);
    if (success) {
        QByteArray kept;
        bool removed = false;
        while (!menu.atEnd()) {
            const QByteArray line = menu.readLine();
            if (DockFileParser::menuEntryName(QString::fromUtf8(line), filePath).isEmpty()) {
                kept += line;
            } else {
                removed = true;
            }
        }
        success = menu.error() == QFileDevice::NoError;
        menu.close();
        if (success && removed) {
            success = writeMenu(menu.fileName(), kept);
        }
    } else {
        std::error_code error;
        success = !std::filesystem::exists(menu.filesystemFileName(), error) && !error;
    }
    if (!success) {
        setLastError(tr("Failed to remove dock from Fluxbox menu"));
        emit operationError(operation, m_lastError);
        emit operationCompleted(operation, false);
        return false;
    }

    emit operationCompleted(operation, true);
    return true;
}

bool DockFileManager::isInMenu(const QString &filePath) const
{
    return !DockFileParser::extractDockName(filePath).isEmpty();
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

bool DockFileManager::writeDockFile(const QString &path, const QString &content, bool makeExecutable)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        setLastError(tr("Could not open file for writing: %1").arg(path));
        return false;
    }

    const QByteArray bytes = content.toUtf8();
    if (file.write(bytes) != bytes.size()) {
        setLastError(file.errorString());
        return false;
    }
    if (makeExecutable && !file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner
                                               | QFile::ReadGroup | QFile::ExeGroup | QFile::ReadOther | QFile::ExeOther)) {
        setLastError(tr("Failed to set executable permissions"));
        return false;
    }
    if (!file.commit()) {
        setLastError(file.errorString());
        return false;
    }
    return true;
}

bool DockFileManager::ensureScriptsDirectory()
{
    QString scriptsDir = QDir::homePath() + PathConstants::FLUXBOX_SCRIPTS_DIR;
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
    return QDir::homePath() + PathConstants::FLUXBOX_SCRIPTS_DIR;
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
    out << "pkill -x wmalauncher\n\n";

    // Add slit location setup
    QString slitLocation = configuration.getSlitLocation();
    if (!slitLocation.isEmpty()) {
        out << "#set up slit location\n";
        out << "sed -i 's/^session.screen0.slit.placement:.*/session.screen0.slit.placement: " << slitLocation << "/' "
            << PathConstants::FLUXBOX_INIT_FILE << "\n\n";
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
            command = "--desktop-file " + ShellArguments::quote(app.appName);
        } else {
            const QString iconPath
                = app.customIcon.isEmpty() ? QStringLiteral("application-x-executable") : app.customIcon;
            QStringList arguments;
            QStringList words;
            if (!ShellArguments::split(app.command, arguments, m_lastError, &words)) {
                return {};
            }
            command = "--command " + words.join(QLatin1Char(' ')) + " --icon " + ShellArguments::quote(iconPath);
        }

        QStringList options;
        QStringList words;
        if (!ShellArguments::split(app.extraOptions, options, m_lastError, &words)) {
            return {};
        }
        const QString extraOptions = options.isEmpty() ? QString() : QLatin1Char(' ') + words.join(QLatin1Char(' '));

        QString tooltipOption;
        if (!app.tooltip.isEmpty()) {
            tooltipOption = QStringLiteral(" --tooltip-text ") + ShellArguments::quote(app.tooltip);
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

void DockFileManager::setLastError(const QString &error)
{
    m_lastError = error;
}

void DockFileManager::clearLastError()
{
    m_lastError.clear();
}

void DockFileManager::restartDockAsync(const QString &processName, const QString &dockFile)
{
    m_processPool.start([processName, dockFile]() {
        killAndWaitForProcess(processName);
        QProcess::startDetached(dockFile, {});
    });
}

void DockFileManager::killAndWaitForProcess(const QString &processName)
{
    // Use pkill to terminate the process
    QProcess pkill;
    pkill.start(QStringLiteral("pkill"), {QStringLiteral("-x"), processName});
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
    QProcess::execute(QStringLiteral("pkill"), {QStringLiteral("-9"), QStringLiteral("-x"), processName});
    QThread::msleep(200); // Give it time to die
}
