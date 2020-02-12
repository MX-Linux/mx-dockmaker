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

#include <QButtonGroup>
#include <QFile>
#include <QMap>
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

    void setup();

public slots:

private slots:
    void addApp(const QString &app);
    void blockAllSignals(bool enable);
    void cleanup();
    void detectSlitLocation();
    void enableAdd();
    void enableNext();
    void parseFile(QFile &file);
    void resetAdd();
    void showApp(int i);

    void on_buttonSave_clicked();
    void on_buttonAbout_clicked();
    void on_buttonHelp_clicked();

    void on_comboBgColor_currentIndexChanged(const QString);
    void on_comboBorderColor_currentIndexChanged(const QString);
    void on_comboSize_currentIndexChanged(const QString);
    void on_buttonNext_clicked();
    void on_buttonDelete_clicked();
    void on_buttonSelect_clicked();
    void on_radioEditDock_toggled(bool checked);
    void onGroupButton(int button_id);
    void on_radioNewDock_toggled(bool checked);
    void on_buttonPrev_clicked();

private:
    Ui::MainWindow *ui;
    QProcess proc;
    Cmd cmd;

    int index = 0;
    QButtonGroup *buttonGroup;
    QVector<QStringList> apps;

};


#endif

