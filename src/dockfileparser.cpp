/**********************************************************************
 *  dockfileparser.cpp
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

#include "dockfileparser.h"
#include "pathconstants.h"
#include "shellarguments.h"

#include <algorithm>

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

const QSet<QString> DockFileParser::KNOWN_OPTIONS = {"-d",
                                                     "--desktop-file",
                                                     "-c",
                                                     "--command",
                                                     "-i",
                                                     "--icon",
                                                     "-k",
                                                     "--background-color",
                                                     "-K",
                                                     "--hover-background-color",
                                                     "-b",
                                                     "--border-color",
                                                     "-B",
                                                     "--hover-border-color",
                                                     "-w",
                                                     "--window-size",
                                                     "--tooltip-text",
                                                     "-x",
                                                     "--exit-on-right-click"};

// Static regex patterns to avoid recompilation
static const QRegularExpression wmalauncherPrefixRe(QStringLiteral("^wmalauncher"));
static const QRegularExpression sleepSuffixRe(QStringLiteral(R"(\s*&(?:\s*sleep\s+[0-9.]+\s*;?)?$)"));

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
    } else {
        emit parsingError(m_lastError);
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
            if (!m_lastError.isEmpty()) {
                return false;
            }
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
    const QString locationsPattern = POSSIBLE_LOCATIONS.join(QLatin1Char('|'));
    const QRegularExpression placementPattern(
        QStringLiteral(R"(session\.screen0\.slit\.placement:\s*(%1)\b)").arg(locationsPattern));
    const QRegularExpressionMatch match = placementPattern.match(content);
    return match.hasMatch() ? match.captured(1) : QString();
}

QString DockFileParser::menuEntryName(const QString &line, const QString &filePath)
{
    // Older sed-based writers emitted a literal t before the two indentation tabs.
    static const QRegularExpression entry(QStringLiteral(R"(^\s*(?:t\t\t)?\[exec\]\s*\(([^)]*)\)\s*\{([^}]*)\})"),
                                          QRegularExpression::CaseInsensitiveOption);
    const auto match = entry.match(line);
    if (!match.hasMatch()) {
        return {};
    }

    QString command = match.captured(2).trimmed();
    if (command.startsWith(QLatin1String("~/"))) {
        command = QDir::home().filePath(command.mid(2));
    } else {
        command = stripQuotes(command);
    }
    if (QDir::cleanPath(command) != QDir::cleanPath(QFileInfo(filePath).absoluteFilePath())) {
        return {};
    }
    return stripQuotes(match.captured(1));
}

QString DockFileParser::extractDockName(const QString &fileName)
{
    QFile menu(QDir::homePath() + PathConstants::FLUXBOX_SUBMENUS_FILE);
    if (menu.open(QIODevice::ReadOnly)) {
        while (!menu.atEnd()) {
            const QString name = menuEntryName(QString::fromUtf8(menu.readLine()), fileName);
            if (!name.isEmpty()) {
                return name;
            }
        }
    }
    return {};
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
    QStringList tokens;
    QStringList spellings;
    if (!ShellArguments::split(cleanLine, tokens, m_lastError, &spellings)) {
        return {};
    }
    QStringList extraOptions;
    for (int i = 0; i < tokens.size(); ++i) {
        const QString token = tokens.at(i);

        auto nextValue = [&](QString *out) {
            if (i + 1 < tokens.size()) {
                *out = tokens.at(++i);
            }
        };

        if (token == QLatin1String("-d") || token == QLatin1String("--desktop-file")) {
            QString value;
            nextValue(&value);
            if (!value.isEmpty()) {
                info.appName = value;
            }
        } else if (token == QLatin1String("-c") || token == QLatin1String("--command")) {
            QStringList command;
            if (i + 1 < tokens.size()) {
                command.append(spellings.at(++i));
                // wmalauncher consumes command arguments up to its next --icon/-i.
                // Older dock files without that delimiter use the next launcher option.
                const auto first = tokens.cbegin() + i + 1;
                auto end = std::find_if(first, tokens.cend(), [](const QString &arg) {
                    return arg == QLatin1String("--icon") || arg == QLatin1String("-i");
                });
                if (end == tokens.cend()) {
                    end = std::find_if(first, end, [](const QString &arg) { return isKnownOption(arg, false); });
                }
                command.append(spellings.mid(i + 1, end - first));
                i = static_cast<int>(end - tokens.cbegin()) - 1;
            }
            info.command = command.join(QLatin1Char(' '));
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
            extraOptions.append(spellings.at(i));
        }
    }
    info.extraOptions = extraOptions.join(QLatin1Char(' '));
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

bool DockFileParser::isKnownOption(const QString &token, bool allowExitOnRightClick)
{
    if (!allowExitOnRightClick
        && (token == QLatin1String("-x") || token == QLatin1String("--exit-on-right-click"))) {
        return false;
    }
    return KNOWN_OPTIONS.contains(token);
}

void DockFileParser::setLastError(const QString &error)
{
    m_lastError = error;
}

void DockFileParser::clearLastError()
{
    m_lastError.clear();
}
