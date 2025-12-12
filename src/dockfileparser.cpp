/**********************************************************************
 *  dockfileparser.cpp
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

#include "dockfileparser.h"

#include <QColor>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

// Static constants
const QStringList DockFileParser::POSSIBLE_LOCATIONS({"TopLeft", "TopCenter", "TopRight", "LeftTop", "RightTop",
                                                      "LeftCenter", "RightCenter", "LeftBottom", "RightBottom",
                                                      "BottomLeft", "BottomCenter", "BottomRight"});

const QSet<QString> DockFileParser::KNOWN_OPTIONS = {"-d", "--desktop-file", "-c", "--command", "-i", "--icon", "-k",
                                                   "--background-color", "-K", "--hover-background-color", "-b",
                                                   "--border-color", "-B", "--hover-border-color", "-w",
                                                   "--window-size", "--tooltip-text", "-x", "--exit-on-right-click"};

// Static regex patterns to avoid recompilation
static const QRegularExpression wmalauncherPrefixRe(QStringLiteral("^wmalauncher"));
static const QRegularExpression sleepSuffixRe(QStringLiteral("\\s*&(?:\\s*sleep.*)?$"));
static const QRegularExpression tokenRe(QStringLiteral(R"((\"[^\"]*\"|'[^']*'|\S+))"));
static const QRegularExpression nameCaptureRe(QStringLiteral("\\((.*)\\)"));

DockFileParser::DockFileParser(QObject *parent)
    : QObject(parent)
{
}

bool DockFileParser::parseFile(const QString &filePath, DockConfiguration &configuration)
{
    clearLastError();
    emit parsingStarted();

    QFile file(filePath);
    if (!file.open(QFile::Text | QFile::ReadOnly)) {
        setLastError(tr("Could not open file: %1").arg(filePath));
        emit parsingError(m_lastError);
        emit parsingCompleted(false);
        return false;
    }

    QString content = file.readAll();
    file.close();

    const bool success = parseContent(content, configuration);
    if (success) {
        configuration.setFileName(filePath);
    }

    emit parsingCompleted(success);
    return success;
}

bool DockFileParser::parseContent(const QString &content, DockConfiguration &configuration)
{
    clearLastError();
    configuration.clear();

    QString slitLocation = extractSlitLocation(content);
    if (!slitLocation.isEmpty()) {
        configuration.setSlitLocation(slitLocation);
    }

    QStringList lines = content.split('\n', Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        QString trimmedLine = line.trimmed();

        if (trimmedLine.startsWith(QLatin1String("sed -i"))) {
            // This is a slit relocation command, already handled above
            continue;
        }

        if (trimmedLine.startsWith(QLatin1String("wmalauncher"))) {
            DockIconInfo iconInfo = parseWmalauncherLine(trimmedLine);
            if (iconInfo.isValid()) {
                configuration.addApplication(iconInfo);
            }
        }
    }

    if (!configuration.isValid()) {
        setLastError(tr("No valid applications found in dock file"));
        return false;
    }

    return true;
}

QString DockFileParser::extractSlitLocation(const QString &content) const
{
    for (const QString &location : POSSIBLE_LOCATIONS) {
        if (content.contains(location)) {
            return location;
        }
    }
    return QString();
}

QString DockFileParser::extractDockName(const QString &fileName)
{
    QFileInfo fileInfo(fileName);
    QString baseName = fileInfo.baseName();

    // Try to extract name from submenus file
    QFile submenusFile(QDir::homePath() + "/.fluxbox/submenus/appearance");
    if (submenusFile.open(QFile::Text | QFile::ReadOnly)) {
        QString content = submenusFile.readAll();
        submenusFile.close();

        QRegularExpression fileRe(".*" + baseName + ".*");

        QRegularExpressionMatch fileMatch = fileRe.match(content);
        if (fileMatch.hasMatch()) {
            QString matchedText = fileMatch.captured();
            QRegularExpressionMatch nameMatch = nameCaptureRe.match(matchedText);
            if (nameMatch.hasMatch()) {
                QString name = nameMatch.captured(1);
                if (name.length() >= 2) {
                    return name.mid(1, name.length() - 2); // Remove parentheses
                }
            }
        }
    }

    // Fallback to user input
    return QString();
}

bool DockFileParser::validateFile(const QString &filePath)
{
    clearLastError();

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        setLastError(tr("File does not exist: %1").arg(filePath));
        return false;
    }

    if (!fileInfo.isReadable()) {
        setLastError(tr("File is not readable: %1").arg(filePath));
        return false;
    }

    if (!filePath.endsWith(QLatin1String(".mxdk"))) {
        setLastError(tr("File does not have .mxdk extension: %1").arg(filePath));
        return false;
    }

    // Try to parse the file
    QFile testFile(filePath);
    if (!testFile.open(QFile::Text | QFile::ReadOnly)) {
        setLastError(tr("Cannot open file for validation: %1").arg(filePath));
        return false;
    }
    QString content = testFile.readAll();
    testFile.close();

    DockConfiguration testConfig;
    return parseContent(content, testConfig);
}

QString DockFileParser::getLastError() const
{
    return m_lastError;
}

DockIconInfo DockFileParser::parseWmalauncherLine(const QString &line)
{
    DockIconInfo info;

    QString cleanLine = line;
    cleanLine.remove(wmalauncherPrefixRe);
    cleanLine.remove(sleepSuffixRe);
    auto matchIt = ::tokenRe.globalMatch(cleanLine);

    QStringList tokens;
    while (matchIt.hasNext()) {
        tokens << matchIt.next().captured(0);
    }

    for (int i = 0; i < tokens.size(); ++i) {
        const QString token = tokens.at(i);

        auto nextValue = [&](QString *out) {
            if (i + 1 < tokens.size()) {
                *out = stripQuotes(tokens.at(++i));
            }
        };

        if (token == QLatin1String("-d") || token == QLatin1String("--desktop-file")) {
            QString value;
            nextValue(&value);
            if (!value.isEmpty()) {
                info.appName = value;
            }
        } else if (token == QLatin1String("-c") || token == QLatin1String("--command")) {
            QString value;
            nextValue(&value);
            // Collect remaining non-option tokens as part of command
            while (i + 1 < tokens.size() && !isKnownOption(tokens.at(i + 1))) {
                value += QLatin1Char(' ') + stripQuotes(tokens.at(++i));
            }
            if (!value.isEmpty()) {
                info.command = value;
            }
        } else if (token == QLatin1String("-i") || token == QLatin1String("--icon")) {
            QString value;
            nextValue(&value);
            if (!value.isEmpty()) {
                info.customIcon = value;
            }
        } else if (token == QLatin1String("-k") || token == QLatin1String("--background-color")) {
            QString color;
            nextValue(&color);
            if (!color.isEmpty()) {
                info.backgroundColor = QColor(color);
            }
        } else if (token == QLatin1String("-K") || token == QLatin1String("--hover-background-color")) {
            QString color;
            nextValue(&color);
            if (!color.isEmpty()) {
                info.hoverBackground = QColor(color);
            }
        } else if (token == QLatin1String("-b") || token == QLatin1String("--border-color")) {
            QString color;
            nextValue(&color);
            if (!color.isEmpty()) {
                info.borderColor = QColor(color);
            }
        } else if (token == QLatin1String("-B") || token == QLatin1String("--hover-border-color")) {
            QString color;
            nextValue(&color);
            if (!color.isEmpty()) {
                info.hoverBorder = QColor(color);
            }
        } else if (token == QLatin1String("-w") || token == QLatin1String("--window-size")) {
            QString size;
            nextValue(&size);
            if (!size.isEmpty()) {
                info.size = size + QLatin1Char('x') + size;
            }
        } else if (token == QLatin1String("--tooltip-text")) {
            QString tooltip;
            nextValue(&tooltip);
            // Sanitize tooltip by removing quotes
            tooltip.remove(QLatin1Char('\''));
            tooltip.remove(QLatin1Char('"'));
            info.tooltip = tooltip;
        } else if (token == QLatin1String("-x") || token == QLatin1String("--exit-on-right-click")) {
            // Not used right now, but we could store it if needed
        } else {
            // Unknown option, store as extra options
            QString value;
            if (i + 1 < tokens.size()) {
                const QString possibleValue = tokens.at(i + 1);
                if (!possibleValue.startsWith(QLatin1Char('-')) && !isKnownOption(possibleValue)) {
                    value = possibleValue;
                    ++i;
                }
            }

            if (!info.extraOptions.isEmpty()) {
                info.extraOptions += QLatin1Char(' ');
            }
            info.extraOptions += token;
            if (!value.isEmpty()) {
                info.extraOptions += QLatin1Char(' ') + value;
            }
        }
    }

    return info;
}

QString DockFileParser::stripQuotes(const QString &value)
{
    if (value.size() >= 2) {
        if ((value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"')))
            || (value.startsWith(QLatin1Char('\'')) && value.endsWith(QLatin1Char('\'')))) {
            return value.mid(1, value.size() - 2);
        }
    }
    return value;
}

bool DockFileParser::isKnownOption(const QString &token)
{
    return KNOWN_OPTIONS.contains(stripQuotes(token));
}

void DockFileParser::setLastError(const QString &error)
{
    m_lastError = error;
    emit parsingError(error);
}

void DockFileParser::clearLastError()
{
    m_lastError.clear();
}
