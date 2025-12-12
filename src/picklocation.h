/**********************************************************************
 *  picklocation.h
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

#include <QButtonGroup>
#include <QDialog>

namespace Ui
{
class PickLocation;
}

class PickLocation : public QDialog
{
    Q_OBJECT

public:
    explicit PickLocation(const QString &location, QWidget *parent = nullptr);
    ~PickLocation() override;
    QString button;

private slots:
    void onGroupButton(int buttonId);

private:
    Ui::PickLocation *ui;
    QButtonGroup *buttonGroup;
};
