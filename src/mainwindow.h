/**********************************************************************
 *  mainwindow.h
 **********************************************************************
 * Copyright (C) 2020 MX Authors
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFile>
#include <QMessageBox>
#include <QMouseEvent>
#include <QSettings>

#include "cmd.h"

namespace Ui
{
class MainWindow;
}

class MainWindow : public QDialog
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr, const QString &file = QString());
    ~MainWindow() override;

    enum Info { App, Command, Icon, Size, BgColor, BgHoverColor, BorderColor, BorderHoverColor, Extra };
    bool checkDoneEditing();
    bool isDockInMenu(const QString &file_name);

    void addDockToMenu(const QString &file_name);
    void blockComboSignals(bool block);
    void deleteDock();
    void displayIcon(const QString &app_name, int location);
    void editDock(const QString &file_arg = QString());
    void moveDock();
    void moveIcon(int pos);
    void newDock();
    void parseFile(QFile &file);
    void resetAdd();
    void setConnections();
    void setup(const QString &file = QString());
    void showApp(int i, int old_idx);
    void updateAppList(int idx);

    QPixmap findIcon(QString icon_name, QSize size);
    QString pickSlitLocation();
    static QString getDockName(const QString &file_name);
    static QString inputDockName();

public slots:

private slots:
    void allItemsChanged();
    void buttonAbout_clicked();
    void buttonAdd_clicked();
    void buttonDelete_clicked();
    void buttonHelp_clicked();
    void buttonMoveLeft_clicked();
    void buttonMoveRight_clicked();
    void buttonNext_clicked();
    void buttonPrev_clicked();
    void buttonSave_clicked();
    void buttonSelectApp_clicked();
    void buttonSelectIcon_clicked();
    void checkApplyStyleToAll_stateChanged(int arg1);
    void comboBgColor_currentTextChanged();
    void comboBorderColor_currentTextChanged();
    void comboSize_currentTextChanged();
    void itemChanged();
    void lineEditCommand_textEdited();
    void mousePressEvent(QMouseEvent *event) override;
    void pickColor(QWidget *widget);
    void radioDesktop_toggled(bool checked);
    void setColor(QWidget *widget, const QColor &color);

private:
    Ui::MainWindow *ui;
    QProcess proc;
    Cmd cmd;

    bool changed = false;
    bool parsing = false;
    int index = 0;
    QList<QLabel *> list_icons;
    QString dock_name;
    QString file_content;
    QString file_name;
    QString slit_location;
    QSettings settings;
    QList<QStringList> apps;
};

#endif
