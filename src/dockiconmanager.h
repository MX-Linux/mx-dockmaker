/**********************************************************************
 *  dockiconmanager.h
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

#pragma once

#include <QColor>
#include <QLabel>
#include <QObject>
#include <QPixmap>
#include <QSize>

#include "dockiconinfo.h"

class DockIconManager : public QObject
{
    Q_OBJECT
public:
    QPixmap findIcon(const QString &iconName, QSize size);
    QString getLastError() const;
    explicit DockIconManager(QObject *parent = nullptr);
    static QSize getIconContainerSize(QSize iconSize);
    static QString getDefaultIconSize();
    static QStringList getAvailableIconSizes();
    void applyIconStyle(const DockIconInfo &iconInfo, QLabel *label, bool isSelected = false);
    void displayIcon(const DockIconInfo &iconInfo, QLabel *label, int selectedIndex = -1, int currentIndex = -1);

signals:
    void iconError(const QString &operation, const QString &error);
    void iconSearchCompleted(const QString &iconName, bool found);
    void iconSearchStarted(const QString &iconName);

private:
    QString m_lastError; ///< Last error message

    // Icon styling constants
    QPixmap findFilesystemIcon(const QString &iconName, QSize size);
    QPixmap findThemeIcon(const QString &iconName, QSize size);
    QPixmap searchIconInPaths(const QString &iconName, QSize size);
    QString generateIconStyle(const DockIconInfo &iconInfo, bool isSelected) const;
    QStringList getIconSearchPaths() const;
    static constexpr int ICON_BORDER_WIDTH = 4;
    static constexpr int ICON_PADDING = 4;
    void clearLastError();
    void setLastError(const QString &error);
};
