/**********************************************************************
 *  pathconstants.h
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

#include <QString>
#include <QStringList>

namespace PathConstants
{

// System application paths
inline const QString APPLICATIONS_DIR = QStringLiteral("/usr/share/applications");

// Fluxbox configuration paths (relative to home directory)
inline const QString FLUXBOX_SCRIPTS_DIR = QStringLiteral("/.fluxbox/scripts");
inline const QString FLUXBOX_SUBMENUS_FILE = QStringLiteral("/.fluxbox/submenus/appearance");
inline const QString FLUXBOX_INIT_FILE = QStringLiteral("$HOME/.fluxbox/init");

// Icon search paths
inline const QStringList ICON_SEARCH_PATHS = {
    QStringLiteral("/usr/share/pixmaps/"),
    QStringLiteral("/usr/local/share/icons/"),
    QStringLiteral("/usr/share/icons/"),
    QStringLiteral("/usr/share/icons/hicolor/scalable/apps/"),
    QStringLiteral("/usr/share/icons/hicolor/48x48/apps/"),
    QStringLiteral("/usr/share/icons/Adwaita/48x48/legacy/")
};

// Application icon path
inline const QString APP_ICON_PATH = QStringLiteral("/usr/share/icons/hicolor/192x192/apps/mx-dockmaker.png");

} // namespace PathConstants
