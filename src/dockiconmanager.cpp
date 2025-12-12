/**********************************************************************
 *  dockiconmanager.cpp
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

#include "dockiconmanager.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QPainter>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>

DockIconManager::DockIconManager(QObject *parent)
    : QObject(parent)
{
}

QPixmap DockIconManager::findIcon(const QString &iconName, QSize size)
{
    clearLastError();
    emit iconSearchStarted(iconName);

    if (iconName.isEmpty()) {
        setLastError(tr("Icon name is empty"));
        emit iconSearchCompleted(iconName, false);
        return QPixmap();
    }

    // Check if it's an absolute path
    if (QFileInfo::exists(iconName)) {
        QPixmap pixmap = QIcon(iconName).pixmap(size);
        if (!pixmap.isNull()) {
            emit iconSearchCompleted(iconName, true);
            return pixmap;
        }
    }

    // Try to find icon in theme first
    QPixmap pixmap = findThemeIcon(iconName, size);
    if (!pixmap.isNull()) {
        emit iconSearchCompleted(iconName, true);
        return pixmap;
    }

    // Try filesystem search
    pixmap = findFilesystemIcon(iconName, size);
    if (!pixmap.isNull()) {
        emit iconSearchCompleted(iconName, true);
        return pixmap;
    }

    setLastError(tr("Icon not found: %1").arg(iconName));
    emit iconSearchCompleted(iconName, false);
    return QPixmap();
}

void DockIconManager::displayIcon(const DockIconInfo &iconInfo, QLabel *label, int selectedIndex, int currentIndex)
{
    clearLastError();

    if (!label) {
        setLastError(tr("Invalid label provided"));
        return;
    }

    QString iconPath;
    if (!iconInfo.customIcon.isEmpty()) {
        iconPath = iconInfo.customIcon;
    } else if (iconInfo.isDesktopFile()) {
        // Extract icon from .desktop file
        QFile desktopFile("/usr/share/applications/" + iconInfo.appName);
        if (desktopFile.open(QFile::ReadOnly)) {
            QString content = desktopFile.readAll();
            desktopFile.close();

            QRegularExpression re(QStringLiteral("^Icon=(.*)$"), QRegularExpression::MultilineOption);
            auto match = re.match(content);
            if (match.hasMatch()) {
                iconPath = match.captured(1);
            } else {
                setLastError(tr("Could not find icon in .desktop file: %1").arg(iconInfo.appName));
            }
        } else {
            setLastError(tr("Could not open .desktop file: %1").arg(iconInfo.appName));
        }
    }
    if (iconPath.isEmpty() && !iconInfo.isDesktopFile()) {
        iconPath = QStringLiteral("application-x-executable");
    }

    QSize iconSize = iconInfo.iconSize();
    QPixmap pixmap = findIcon(iconPath, iconSize);

    if (pixmap.isNull()) {
        // Use a default fallback icon
        pixmap = QIcon::fromTheme(QStringLiteral("application-x-executable")).pixmap(iconSize);

        // If theme icon is also missing, create a simple placeholder
        if (pixmap.isNull()) {
            pixmap = QPixmap(iconSize);
            pixmap.fill(Qt::gray);
            // Draw a simple "?" icon
            QPainter painter(&pixmap);
            painter.setPen(Qt::white);
            painter.setFont(QFont("Sans", iconSize.height() / 2));
            painter.drawText(pixmap.rect(), Qt::AlignCenter, "?");
        }
    }

    QSize containerSize = getIconContainerSize(iconSize);
    label->setPixmap(pixmap);
    label->setAlignment(Qt::AlignCenter);
    label->setFixedSize(containerSize);
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // Apply styling
    bool isSelected = (selectedIndex >= 0 && currentIndex == selectedIndex);
    applyIconStyle(iconInfo, label, isSelected);

    // Store tooltip
    if (!iconInfo.tooltip.isEmpty()) {
        label->setProperty("icon_tooltip", iconInfo.tooltip);
    }
}

void DockIconManager::applyIconStyle(const DockIconInfo &iconInfo, QLabel *label, bool isSelected)
{
    if (!label) {
        return;
    }

    QString style = generateIconStyle(iconInfo, isSelected);
    label->setStyleSheet(style);
}

QSize DockIconManager::getIconContainerSize(QSize iconSize)
{
    return iconSize + QSize(2 * (ICON_BORDER_WIDTH + ICON_PADDING), 2 * (ICON_BORDER_WIDTH + ICON_PADDING));
}

QString DockIconManager::getDefaultIconSize()
{
    return QStringLiteral("48x48");
}

QStringList DockIconManager::getAvailableIconSizes()
{
    return {QStringLiteral("16x16"), QStringLiteral("24x24"), QStringLiteral("32x32"),  QStringLiteral("48x48"),
            QStringLiteral("64x64"), QStringLiteral("96x96"), QStringLiteral("128x128")};
}

QString DockIconManager::getLastError() const
{
    return m_lastError;
}

QPixmap DockIconManager::searchIconInPaths(const QString &iconName, QSize size)
{
    // Try theme icon first
    QPixmap pixmap = findThemeIcon(iconName, size);
    if (!pixmap.isNull()) {
        return pixmap;
    }

    // Try filesystem search
    return findFilesystemIcon(iconName, size);
}

QPixmap DockIconManager::findThemeIcon(const QString &iconName, QSize size)
{
    // Remove file extensions for theme lookup
    QString cleanName = iconName;
    cleanName.remove(QRegularExpression(QStringLiteral(R"(\.png$|\.svg$|\.xpm$)")));

    // Try to get icon from theme
    QIcon themeIcon = QIcon::fromTheme(cleanName);
    if (!themeIcon.isNull()) {
        return themeIcon.pixmap(size);
    }

    return QPixmap();
}

QPixmap DockIconManager::findFilesystemIcon(const QString &iconName, QSize size)
{
    QStringList searchPaths = getIconSearchPaths();

    // Remove extensions for exact matching
    QString cleanName = iconName;
    cleanName.remove(QRegularExpression(QStringLiteral(R"(\.png$|\.svg$|\.xpm$)")));

    // Search in standard paths
    for (const QString &path : searchPaths) {
        if (!QFileInfo::exists(path)) {
            continue;
        }

        for (const QString &ext : {".png", ".svg", ".xpm"}) {
            QString fullPath = path + cleanName + ext;
            if (QFileInfo::exists(fullPath)) {
                return QIcon(fullPath).pixmap(size);
            }
        }
    }

    // Use recursive search as last resort (more expensive)
    // Helper function to search for icon in all paths
    const auto searchInPaths = [&](const QString &name) -> QIcon {
        for (const auto &path : searchPaths) {
            const QString fullPath = QDir(path).filePath(name);
            if (QFile::exists(fullPath)) {
                QIcon icon(fullPath);
                if (!icon.isNull()) {
                    return icon;
                }
            }
        }
        return QIcon();
    };

    // First, try with the plain icon name (no extension)
    QIcon foundIcon = searchInPaths(cleanName);
    if (!foundIcon.isNull()) {
        return foundIcon.pixmap(size);
    }

    // Then, try with common icon extensions
    for (const QString &ext : {".png", ".svg", ".xpm"}) {
        QIcon iconWithExt = searchInPaths(cleanName + ext);
        if (!iconWithExt.isNull()) {
            return iconWithExt.pixmap(size);
        }
    }

    return QPixmap();
}

QStringList DockIconManager::getIconSearchPaths()
{
    return {QDir::homePath() + "/.local/share/icons/",
            "/usr/share/pixmaps/",
            "/usr/local/share/icons/",
            "/usr/share/icons/",
            "/usr/share/icons/hicolor/scalable/apps/",
            "/usr/share/icons/hicolor/48x48/apps/",
            "/usr/share/icons/Adwaita/48x48/legacy/"};
}

QString DockIconManager::generateIconStyle(const DockIconInfo &iconInfo, bool isSelected)
{
    QString style
        = QStringLiteral("background-color: %1; padding: %2px;").arg(iconInfo.backgroundColor.name()).arg(ICON_PADDING);

    if (isSelected) {
        style += QStringLiteral("border: %1px dotted #0078d4;").arg(ICON_BORDER_WIDTH);
    } else {
        style += QStringLiteral("border: %1px solid %2;").arg(ICON_BORDER_WIDTH).arg(iconInfo.borderColor.name());
    }

    return style;
}

void DockIconManager::setLastError(const QString &error)
{
    m_lastError = error;
    emit iconError(tr("Icon operation"), error);
}

void DockIconManager::clearLastError()
{
    m_lastError.clear();
}
