/**********************************************************************
 *  about.h
 **********************************************************************
 * Copyright (C) 2020-2025 MX Authors
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

#pragma once

class QString;
class QWidget;

[[nodiscard]] QString docPath(const QString &packageName, const QString &fileName);
void displayDoc(QWidget *parent, const QString &path, const QString &title, bool largeWindow = false);
void displayAboutMsgBox(QWidget *parent, const QString &title, const QString &message, const QString &licensePath,
                        const QString &licenseTitle);
