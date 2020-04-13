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

#include "cmd.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QDialog
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool isDockInMenu(const QString &file_name);

    void addDockToMenu(const QString &file_name);
    void blockComboSignals(bool enable);
    void cleanup();
    void deleteDock();
    void displayIcon(const QString &app_name, int location);
    void ifNotDoneDisableButtons();
    void editDock();
    void enableNext();
    void moveDock();
    void newDock();
    void parseFile(QFile &file);
    void resetAdd();
    void setup();
    void showApp(int i);
    void updateAppList(int idx);

    QString findIcon(const QString &icon_name);
    QString findLargest(const QStringList &files);
    QString getDockName(const QString &file_name);
    QString inputDockName();
    QString pickSlitLocation();

public slots:

private slots:
    void itemChanged();
    void on_buttonSave_clicked();
    void on_buttonAbout_clicked();
    void on_buttonHelp_clicked();

    void on_buttonDelete_clicked();
    void on_buttonNext_clicked();
    void on_buttonPrev_clicked();
    void on_buttonSelectApp_clicked();
    void on_buttonSelectIcon_clicked();
    void on_comboBgColor_currentIndexChanged(const QString);
    void on_comboBorderColor_currentIndexChanged(const QString);
    void on_comboSize_currentIndexChanged(const QString);
    void on_radioDesktop_toggled(bool checked);
    void on_lineEditCommand_textEdited(const QString);

private:
    Ui::MainWindow *ui;
    QProcess proc;
    Cmd cmd;

    bool changed = false;
    int index = 0;
    QList<QLabel *> list_icons;
    QString dock_name;
    QString file_name;
    QString slit_location;
    QList<QStringList> apps;


};


#endif

