/**********************************************************************
 *  mainwindow.cpp
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

#include <QButtonGroup>
#include <QCommandLineParser>
#include <QDebug>
#include <QFileDialog>


#include <about.h>
#include <cmd.h>
#include <sys/stat.h>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "version.h"

MainWindow::MainWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MainWindow)
{
    qDebug() << "Program Version:" << VERSION;
    ui->setupUi(this);
    setWindowFlags(Qt::Window); // for the close, min and max buttons
    setup();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// setup versious items first time program runs
void MainWindow::setup()
{
    connect(qApp, &QApplication::aboutToQuit, this, &MainWindow::cleanup);
    this->setWindowTitle("MX Dockmaker");
    this->adjustSize();

    blockAllSignals(true);

    ui->comboSize->setCurrentIndex(ui->comboSize->findText("48x48"));

    QStringList color_list({"apricot", "beige", "black", "blue", "brown", "cyan", "gray", "green",
                           "lavender", "lime", "magenta", "maroon", "mint", "navy", "olive",
                           "orange", "pink", "purple", "red", "teal", "white", "yellow"});

    ui->comboBgColor->addItems(color_list);
    ui->comboBorderColor->addItems(color_list);

    ui->comboBgColor->setCurrentIndex(ui->comboBgColor->findText("white"));
    ui->comboBorderColor->setCurrentIndex(ui->comboBorderColor->findText("white"));

    blockAllSignals(false);

    buttonGroup = new QButtonGroup(this);
    buttonGroup->addButton(ui->buttonTL, 1);
    buttonGroup->addButton(ui->buttonTC, 2);
    buttonGroup->addButton(ui->buttonTR, 3);
    buttonGroup->addButton(ui->buttonLC, 4);
    buttonGroup->addButton(ui->buttonRC, 5);
    buttonGroup->addButton(ui->buttonBL, 6);
    buttonGroup->addButton(ui->buttonBC, 7);
    buttonGroup->addButton(ui->buttonBR, 8);
    buttonGroup->addButton(ui->buttonLT, 9);
    buttonGroup->addButton(ui->buttonLB, 10);
    buttonGroup->addButton(ui->buttonRT, 11);
    buttonGroup->addButton(ui->buttonRB, 12);

    detectSlitLocation();
    connect(buttonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), this, &MainWindow::onGroupButton);

}

void MainWindow::addApp(const QString &app)
{
    apps.push_back(QStringList({app, ui->comboSize->currentText(), ui->comboBgColor->currentText(), ui->comboBorderColor->currentText(), QString::number(ui->cbFreze->isChecked())}));
    //qDebug() << apps;
}

// cleanup environment when window is closed
void MainWindow::cleanup()
{

}

// block/unblock all relevant signals when loading stuff into GUI
void MainWindow::blockAllSignals(bool block)
{
    ui->comboSize->blockSignals(block);
    ui->comboBgColor->blockSignals(block);
    ui->comboBorderColor->blockSignals(block);
}

// detect slit location and click the button
void MainWindow::detectSlitLocation()
{
    QString location = cmd.getCmdOut("grep ^session.screen0.slit.placement $HOME/.fluxbox/init | cut -f2 -d:", true);

    for (QAbstractButton *button : buttonGroup->buttons()) {
        if (location == button->text()) {
            button->click();
            return;
        }
    }
}

void MainWindow::enableAdd()
{
    ui->buttonNext->setIcon(QIcon::fromTheme("forward"));
    ui->buttonNext->setText(tr("Add application"));
    ui->buttonDelete->setText(tr("Delete last added application"));
}

void MainWindow::enableNext()
{
    ui->buttonNext->setIcon(QIcon::fromTheme("next"));
    ui->buttonNext->setText(tr("Next"));
    ui->buttonNext->setEnabled(true);
    ui->buttonDelete->setText(tr("Delete current entry"));
}


void MainWindow::parseFile(QFile &file)
{
    QString line;
    QCommandLineParser parser;
    parser.addOptions({{{"d", "desktop-file"}, "", "d"},
                       {{"k", "background-color"}, "", "k"},
                       {{"b", "border-color"}, "", "b"},
                       {{"w", "window-size"}, "", "w"},
                       {{"x", "exit-on-right-click"}, "", "x"}});
    while (!file.atEnd()) {
        line = file.readLine().trimmed();
        if (line.startsWith("wmalauncher")) {
            ui->buttonNext->setEnabled(true);
            parser.process(line.split(" "));
            ui->buttonSelect->setText(parser.value("desktop-file"));
            ui->comboSize->setCurrentIndex(ui->comboSize->findText(parser.value("window-size") + "x" + parser.value("window-size")));
            ui->comboBgColor->setCurrentIndex(ui->comboBgColor->findText(parser.value("background-color")));
            ui->comboBorderColor->setCurrentIndex(ui->comboBorderColor->findText(parser.value("border-color")));
            ui->buttonNext->click();
        }
    }
    showApp(index = 0);
}


// Next button clicked
void MainWindow::on_buttonSave_clicked()
{
    // create "~/.fluxbox/scripts" if it doesn't exist
    if(!QFileInfo::exists(QDir::homePath() + "/.fluxbox/scripts")) {
        QDir dir;
        dir.mkpath(QDir::homePath() + "/.fluxbox/scripts");
    }

    // open file
    QString newfile = QFileDialog::getSaveFileName(this, tr("Save file"), QDir::homePath() + "/.fluxbox/scripts" ,tr("Fluxbox Dock Files (*.fbxdock)"));
    if (newfile.isEmpty()) return;
    if (!newfile.endsWith(".fbxdock")) {
        newfile += ".fbxdock";
    }
    QFile file(newfile);
    if(!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Could not open file:" << file.fileName();
        return;
    }
    QTextStream stream(&file);

    // build and write string
    stream << "#!/bin/bash\n\n";
    stream << "# commands for dock launchers\n";

    for (int i = 0; i < apps.size(); ++i) {
        QString freeze = apps.at(i).at(4) == "1" ? "FREEZE" : "sleep 0.1";
        stream << "wmalauncher --desktop-file " + apps.at(i).at(0) + " --background-color " + apps.at(i).at(2) + " --border-color " + apps.at(i).at(3) + " --window-size " + apps.at(i).at(1).section("x", 0, 0) + " -x & " + freeze + "\n";
    }
    file.close();
    chmod(newfile.toUtf8(), 00744);

    QMessageBox::information(this, tr("Dock saved"), tr("The dock has been saved.\n\n"
                                                        "To edit the newly created dock please select 'Edit an existing dock'. "
                                                        "Otherwise continue creatring a new dock."));

    index = 0;
    apps.clear();
    resetAdd();
    ui->buttonSave->setEnabled(false);
    ui->buttonDelete->setEnabled(false);
    ui->radioNewDock->click();
}


// About button clicked
void MainWindow::on_buttonAbout_clicked()
{
    this->hide();
    displayAboutMsgBox( tr("About %1") + "MX Dockmaker",
                       "<p align=\"center\"><b><h2>MX Dockmaker</h2></b></p><p align=\"center\">" +
                       tr("Version: ") + VERSION + "</p><p align=\"center\"><h3>" +
                       tr("Description goes here") +
                       "</h3></p><p align=\"center\"><a href=\"http://mxlinux.org\">http://mxlinux.org</a><br /></p><p align=\"center\">" +
                       tr("Copyright (c) MX Linux") + "<br /><br /></p>",
                        "/usr/share/doc/mx-dockmaker/license.html", tr("%1 License").arg(this->windowTitle()), false);

    this->show();
}

// Help button clicked
void MainWindow::on_buttonHelp_clicked()
{
    QString url = "https://mxlinux.org/wiki/help-files/help-dockmaker/";
    displayDoc(url, tr("%1 Help").arg(this->windowTitle()), false);
}


void MainWindow::on_comboSize_currentIndexChanged(const QString)
{
    //ui->buttonNext->setEnabled(true);
}

void MainWindow::on_comboBgColor_currentIndexChanged(const QString)
{
    //ui->buttonNext->setEnabled(true);
}

void MainWindow::on_comboBorderColor_currentIndexChanged(const QString)
{
    //ui->buttonNext->setEnabled(true);
}


void MainWindow::on_buttonNext_clicked()
{
    blockAllSignals(true);
    index += 1;
    if (index <= apps.size()) {
        ui->buttonDelete->setEnabled(true);
        ui->buttonPrev->setEnabled(true);
        if (index < apps.size()) {
            showApp(index);
        } else {
            enableAdd();
            resetAdd();
        }
    } else {
        enableAdd();
        if (ui->buttonSelect->text() != tr("Select...")) {
            ui->buttonSave->setEnabled(true);
            ui->buttonDelete->setEnabled(true);
            ui->buttonPrev->setEnabled(true);
            addApp(ui->buttonSelect->text());
            resetAdd();
        } else {
            QMessageBox::critical(this, windowTitle(), tr("Please select a file."));
            index -= 1;
            return;
        }
    }
}

void MainWindow::on_buttonDelete_clicked()
{
    if (!apps.isEmpty()) {
        blockAllSignals(true);

        if (ui->buttonDelete->text() == tr("Delete last added application")) {
            apps.pop_back();
            index -= 1; // decrement index because buttonBack doesn't decreament when showing "Delete last added application"
        } else {
            apps.remove(index);
        }

        blockAllSignals(false);
    }
    if (apps.isEmpty()) {
        index = 0;
        ui->buttonDelete->setEnabled(false);
        ui->buttonSave->setEnabled(false);
        enableAdd();
        ui->buttonPrev->setEnabled(false);
        resetAdd();
    } else if (index == 0){
        ui->buttonPrev->setEnabled(false);
        index = -1; // Next button advances index;
        ui->buttonNext->click();
    } else {
        ui->buttonSave->setEnabled(true);
        on_buttonPrev_clicked();
    }
    blockAllSignals(false);
}

void MainWindow::resetAdd()
{
    ui->buttonSelect->setText(tr("Select..."));
    ui->comboSize->setCurrentIndex(ui->comboSize->findText("48x48"));
    ui->comboBgColor->setCurrentIndex(ui->comboBgColor->findText("white"));
    ui->comboBorderColor->setCurrentIndex(ui->comboBorderColor->findText("white"));
    ui->buttonNext->setEnabled(false);
}

void MainWindow::showApp(int idx)
{
    ui->buttonSelect->setText(apps.at(idx).at(0));
    ui->comboSize->setCurrentIndex(ui->comboSize->findText(apps.at(idx).at(1)));
    ui->comboBgColor->setCurrentIndex(ui->comboBgColor->findText(apps.at(idx).at(2)));
    ui->comboBorderColor->setCurrentIndex(ui->comboBorderColor->findText(apps.at(idx).at(3)));
    if (index == 0) {
        ui->buttonPrev->setEnabled(false);
        if (index < apps.size()) {
            enableNext();
        }
    }
}


void MainWindow::on_buttonSelect_clicked()
{
    QString selected = QFileDialog::getOpenFileName(this, tr("Select .desktop file"), "/usr/share/applications", tr("Desktop Files (*.desktop)"));
    QString file = QFileInfo(selected).baseName();
    if (!file.isEmpty()) {
        ui->buttonSelect->setText(file);
        ui->buttonNext->setEnabled(true);
    }
}

void MainWindow::on_radioEditDock_toggled(bool checked)
{
    if (checked) {
        QString selected_dock = QFileDialog::getOpenFileName(this, tr("Select a .fbxdock file"), QDir::homePath() + "/.fluxbox/scripts" ,tr("Fluxbox Dock Files (*.fbxdock);; All Files(*.*)"));
        if (!QFileInfo::exists(selected_dock)) {
            QMessageBox::warning(this, tr("No file selected"), tr("You haven't selected any dock file to edit.\nCreating a new dock instead."));
            ui->radioNewDock->click();
            return;
        }
        QFile file(selected_dock);
        if(!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Could not open file:" << file.fileName();
            QMessageBox::warning(this, tr("Could not open file"), tr("Could not open selected file.\nCreating a new dock instead."));
            ui->radioNewDock->click();
            return;
        }
        parseFile(file);
        file.close();
    }
}

void MainWindow::onGroupButton(int button_id)
{
    QString location = buttonGroup->button(button_id)->text();
    cmd.run("sed -i 's/^session.screen0.slit.placement:.*/session.screen0.slit.placement:" + location + "/' " + QDir::homePath() + "/.fluxbox/init", true);
    system("fluxbox-remote restart");
}

void MainWindow::on_radioNewDock_toggled(bool checked)
{
    if (checked) {
        apps.clear();
        resetAdd();
        ui->buttonSave->setEnabled(false);
        ui->buttonDelete->setEnabled(false);
    }
}

void MainWindow::on_buttonPrev_clicked()
{
    blockAllSignals(true);
    index -= 1;
    enableNext();

    showApp(index);
    if (index == 0) {
        ui->buttonPrev->setEnabled(false);
    }
    if (index == apps.size()) {
        ui->buttonNext->setIcon(QIcon::fromTheme("forward"));
        ui->buttonNext->setText(tr("Add application"));
    }
    blockAllSignals(false);
}
