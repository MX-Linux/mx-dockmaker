/**********************************************************************
 *  dockiconinfo.h
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
#include <QSize>
#include <QString>

struct DockIconInfo {
    // Application information
    QString appName;    ///< Name of the application or .desktop file
    QString command;    ///< Command to execute
    QString customIcon; ///< Custom icon path (empty if using theme icon)
    QString tooltip;    ///< Tooltip text to display

    // Visual properties
    QColor backgroundColor = Qt::black;     ///< Normal background color
    QColor borderColor = Qt::white;         ///< Normal border color
    QColor hoverBackground = Qt::black;     ///< Hover background color
    QColor hoverBorder = Qt::white;         ///< Hover border color
    QString size = QStringLiteral("48x48"); ///< Icon size (e.g., "48x48")

    // Additional options
    QString extraOptions; ///< Additional command-line options

    DockIconInfo() = default;
    DockIconInfo(const QString &app, const QString &cmd)
        : appName(app),
          command(cmd)
    {
    }
    bool isDesktopFile() const
    {
        return appName.endsWith(QLatin1String(".desktop"));
    }
    bool isValid() const
    {
        if (isDesktopFile()) {
            return !appName.isEmpty();
        }
        return !command.isEmpty();
    }
    QSize iconSize() const
    {
        const quint8 width = size.section(QLatin1Char('x'), 0, 0).toUShort();
        return QSize(width, width);
    }
    QStringList toStringList() const
    {
        return {appName,
                command,
                tooltip,
                customIcon,
                size,
                backgroundColor.name(),
                hoverBackground.name(),
                borderColor.name(),
                hoverBorder.name(),
                extraOptions};
    }
    static DockIconInfo fromStringList(const QStringList &list)
    {
        DockIconInfo info;
        if (list.size() >= 11) {
            info.appName = list.at(0);
            info.command = list.at(1);
            info.tooltip = list.at(2);
            info.customIcon = list.at(3);
            info.size = list.at(4);
            info.backgroundColor = QColor(list.at(5));
            info.hoverBackground = QColor(list.at(6));
            info.borderColor = QColor(list.at(7));
            info.hoverBorder = QColor(list.at(8));
            info.extraOptions = list.at(9);
        }
        return info;
    }
};
