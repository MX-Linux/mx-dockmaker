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

#include <QCommandLineParser>
#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>
#include <QTimer>

#include <about.h>
#include <cmd.h>
#include <sys/stat.h>
#include "mainwindow.h"
#include "picklocation.h"
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

bool MainWindow::isDockInMenu(const QString &file_name)
{
    if (dock_name.isEmpty()) return false;
    return getDockName(file_name) == dock_name;
}

void MainWindow::displayIcon(const QString &app_name, int location)
{
    QPalette pal = palette();
    QString icon;
    if (ui->buttonSelectApp->text().endsWith(".desktop")) {
        icon = cmd.getCmdOut("grep -m1 ^Icon= /usr/share/applications/" + app_name + "|cut -f2 -d=", true);
    } else {
        icon = ui->buttonSelectIcon->text();
    }
    int width = ui->comboSize->currentText().section("x", 0, 0).toInt();
    int height = width;
    QSize size(width, height);
    QPixmap pix = QPixmap(findIcon(icon)).scaled(size);
    if (location == list_icons.size()) {
        list_icons << new QLabel(this);
    }
    list_icons.at(location)->setPixmap(pix);
    list_icons.at(location)->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->groupPreview->layout()->addWidget(list_icons.last());
    list_icons.at(location)->setStyleSheet("background-color: " + ui->comboBgColor->currentText() + ";border: 4px solid " + ui->comboBorderColor->currentText() + ";");
}

void MainWindow::ifNotDoneDisableButtons()
{
    if (ui->buttonSelectApp->text() != tr("Select...") || !ui->lineEditCommand->text().isEmpty()) {
        ui->buttonSave->setEnabled(true);
        if (index != 0) ui->buttonPrev->setEnabled(true);
        ui->buttonNext->setEnabled(true);
    } else {
        ui->buttonSave->setEnabled(false);
        ui->buttonPrev->setEnabled(false);
        ui->buttonNext->setEnabled(false);
    }

    if(ui->buttonNext->text() == tr("Add application") && !ui->buttonNext->isEnabled()) {
        ui->buttonDelete->setEnabled(false);
    }
}

// setup versious items first time program runs
void MainWindow::setup()
{
    changed = false;
    connect(qApp, &QApplication::aboutToQuit, this, &MainWindow::cleanup);
    this->setWindowTitle("MX Dockmaker");
    ui->labelUsage->setText(tr("1. Add applications to the dock one at a time\n"
                               "2. Select a .desktop file or enter a command for the application you want\n"
                               "3. Select icon attributes for size, background (black is standard) and border\n"
                               "4. Press \"Add application\" to continue or \"Save\" to finish"));
    this->adjustSize();

    blockComboSignals(true);

    while (ui->groupPreview->layout()->count() > 0) {
        delete ui->groupPreview->layout()->itemAt(0)->widget();
    }
    ui->comboSize->setCurrentIndex(ui->comboSize->findText("48x48"));

    // "apricot", "mint" removed because not availalbe in Qt, might add back if needed
    QStringList color_list({"beige", "black", "blue", "brown", "cyan", "gray", "green",
                           "lavender", "lime", "magenta", "maroon", "navy", "olive",
                           "orange", "pink", "purple", "red", "teal", "white", "yellow"});

    ui->comboBgColor->addItems(color_list);
    ui->comboBorderColor->addItems(color_list);

    ui->comboBgColor->setCurrentIndex(ui->comboBgColor->findText(tr("black")));
    ui->comboBorderColor->setCurrentIndex(ui->comboBorderColor->findText(tr("white")));

    blockComboSignals(false);

    QMessageBox *mbox = new QMessageBox(nullptr);
    mbox->setText(tr("This tool allows you to create a new dock with one or more applications. You can also edit or delete a dock created earlier."));
    mbox->setIcon(QMessageBox::Question);
    mbox->setWindowTitle(tr("Operation mode"));
    mbox->addButton(tr("&Close"), QMessageBox::NoRole);
    mbox->addButton(tr("&Delete"), QMessageBox::NoRole);
    mbox->addButton(tr("&Edit"), QMessageBox::NoRole);
    mbox->addButton(tr("&Create"), QMessageBox::NoRole);

    this->hide();

    switch (mbox->exec()) {
    case 1:
        this->show();
        deleteDock();
        setup();
        break;
    case 2:
        this->show();
        editDock();
        break;
    case 3:
        newDock();
        break;
    default:
        QTimer::singleShot(0, qApp, &QGuiApplication::quit);
    }
    delete mbox;
}

QString MainWindow::findIcon(const QString &icon_name)
{
    if (QFileInfo::exists(icon_name)) return icon_name;

    const QStringList extList({".png", ".svg", ".xpm"});

    QString out = cmd.getCmdOut("xfconf-query -c xsettings -p /Net/IconThemeName", true);
    if (!out.isEmpty()) {
        QString dir = "/usr/share/icons/" + out;
        if (QFileInfo::exists(dir)) {
            for (const QString &ext : extList) {
                out = cmd.getCmdOut("find " + dir + " -iname " + icon_name + ext, true);
                if (!out.isEmpty()) {
                    QStringList files = out.split("\n");
                    return findLargest(files);
                } else {
                    out = cmd.getCmdOut("find " + dir + " /usr/share/icons/hicolor /usr/share/pixmaps -iname " + icon_name + ext, true);
                    if (!out.isEmpty()) {
                        QStringList files = out.split("\n");
                        return findLargest(files);
                    }
                }
            }
        }
    }
    return QString();
}

QString MainWindow::findLargest(const QStringList &files)
{
    qint64 max = 0;
    QString largest;
    for (const QString &file : files) {
        qint64 size = QFile(file).size();
        if (size >= max) {
            largest = file;
            max = size;
        }
    }
    return largest;
}

QString MainWindow::getDockName(const QString &file_name)
{
    return cmd.getCmdOut("grep " + QFileInfo(file_name).baseName() + " " + QDir::homePath() + "/.fluxbox/menu-mx|awk -F\"[()]\" '{print $2}'", true);
}

QString MainWindow::inputDockName()
{
    bool ok;
    QString text = QInputDialog::getText(nullptr, tr("Dock name"),
                                             tr("Enter the name to show in the Menu:"), QLineEdit::Normal,
                                             QString(), &ok);
    if (ok && !text.isEmpty()) return text;

    setup();
    return QString();
}

QString MainWindow::pickSlitLocation()
{
    PickLocation *pick = new PickLocation(slit_location, this);

    pick->exec();
    return pick->button;
}

void MainWindow::itemChanged()
{
    changed = true;
    updateAppList(index);

    ifNotDoneDisableButtons();

    if (!list_icons.empty()) {
        displayIcon(ui->buttonSelectApp->text(), index);
        list_icons.at(index)->setStyleSheet("background-color: " + ui->comboBgColor->currentText() + ";border: 4px solid " + ui->comboBorderColor->currentText() + ";");
    }
}

void MainWindow::updateAppList(int idx)
{
    QStringList app_info = QStringList({ui->buttonSelectApp->text(), ui->lineEditCommand->text(), ui->buttonSelectIcon->text(),
                                        ui->comboSize->currentText(), ui->comboBgColor->currentText(), ui->comboBorderColor->currentText()});
    (idx < apps.size()) ? apps.replace(idx, app_info) : apps.push_back(app_info);
    displayIcon(ui->buttonSelectApp->text(), idx);
}

void MainWindow::addDockToMenu(const QString &file_name)
{
    cmd.run("sed -i '/\\[submenu\\] (Docks & launchers)/a \\\\t\\t\\t[exec] (" + dock_name + ") {" +
            file_name + "}' " + QDir::homePath() + "/.fluxbox/menu-mx", true);
}

// cleanup environment when window is closed
void MainWindow::cleanup()
{

}

void MainWindow::deleteDock()
{
    this->hide();
    QString selected = QFileDialog::getOpenFileName(nullptr, tr("Select dock to delete"), QDir::homePath() + "/.fluxbox/scripts");
    if (!selected.isEmpty() && QMessageBox::question(nullptr, tr("Confirmation"),
                                                     tr("Are you sure you want to delete %1?").arg(selected), tr("&Delete"), tr("&Cancel")) == 0) {
        QFile::remove(selected);
        cmd.run("sed -ni '\\|" + selected + "|!p' " + QDir::homePath() + "/.fluxbox/menu-mx", true);
        cmd.run("pkill wmalauncher", true);
    }
    this->show();
}

// block/unblock all relevant signals when loading stuff into GUI
void MainWindow::blockComboSignals(bool block)
{
    ui->comboSize->blockSignals(block);
    ui->comboBgColor->blockSignals(block);
    ui->comboBorderColor->blockSignals(block);
}

void MainWindow::enableNext()
{
    ui->buttonNext->setIcon(QIcon::fromTheme("go-next"));
    ui->buttonNext->setText(tr("Next"));
    ui->buttonNext->setEnabled(true);
}


void MainWindow::parseFile(QFile &file)
{
    QString line;
    QCommandLineParser parser;

    blockComboSignals(true);

    const QStringList possible_locations({"TopLeft",    "TopCenter",    "TopRight",
                                          "LeftTop",                    "RightTop",
                                          "LeftCenter",                 "RightCenter",
                                          "LeftBottom",                 "RightBottom",
                                          "BottomLeft", "BottomCenter", "BottomRight"});

    parser.addOptions({
                          {{"d", "desktop-file"}, "", "d"},
                          {{"k", "background-color"}, "", "k", "black"},
                          {{"b", "border-color"}, "", "b", "white"},
                          {{"w", "window-size"}, "", "w", "48x48"},
                          {{"x", "exit-on-right-click"}, "", "x"},
                          {{"c", "command"}, "", "c"},
                          {{"i", "icon"}, "", "i"}
                      });
    while (!file.atEnd()) {
        line = file.readLine().trimmed();

        if (line.startsWith("sed -i")) { // assume it's a slit relocation sed command
            for (const QString &location : possible_locations) {
                if (line.contains(location)) {
                    slit_location = location;
                    break;
                }
            }
            continue;
        }
        if (line.startsWith("wmalauncher")) {
            parser.process(line.split(" "));
            if (!parser.value("desktop-file").isEmpty()) {
                ui->radioDesktop->setChecked(true);
                ui->buttonSelectApp->setText(parser.value("desktop-file"));
                ui->buttonSelectIcon->setToolTip(QString());
                ui->buttonSelectIcon->setStyleSheet("text-align: left;");
            } else if (!parser.value("command").isEmpty()) {
                ui->radioCommand->setChecked(true);
                QStringList args = parser.positionalArguments();

                // remove various other positional arguments
                args.removeAll("&");
                args.removeAll("sleep");
                args.removeAll("0.1");

                ui->lineEditCommand->setText(parser.value("command") + (args.size() == 0 ? QString() : " " + args.join(" ")));
                ui->buttonSelectIcon->setText(parser.value("icon"));
                ui->buttonSelectIcon->setToolTip(parser.value("icon"));
                ui->buttonSelectIcon->setStyleSheet("text-align: right;");
            } else {
                qDebug() << "Cannot parse --command line";
            }
            ui->comboSize->setCurrentIndex(ui->comboSize->findText(parser.value("window-size") + "x" + parser.value("window-size")));
            ui->comboBgColor->setCurrentIndex(ui->comboBgColor->findText(parser.value("background-color")));
            ui->comboBorderColor->setCurrentIndex(ui->comboBorderColor->findText(parser.value("border-color")));
            updateAppList(index++);
        }
    }
    changed = false;
    showApp(index = 0);
}


// Next button clicked
void MainWindow::on_buttonSave_clicked()
{
    slit_location = pickSlitLocation();

    // create "~/.fluxbox/scripts" if it doesn't exist
    if(!QFileInfo::exists(QDir::homePath() + "/.fluxbox/scripts")) {
        QDir dir;
        dir.mkpath(QDir::homePath() + "/.fluxbox/scripts");
    }

    if(!QFileInfo::exists(file_name) || QMessageBox::No == QMessageBox::question(this, tr("Overwrite?"), tr("Do you want to overwrite the dock file?"))) {
        file_name = QFileDialog::getSaveFileName(this, tr("Save file"), QDir::homePath() + "/.fluxbox/scripts");
        if (file_name.isEmpty()) return;
    }
    QFile file(file_name);
    cmd.run("cp " + file_name + " " + file_name + ".~", true);
    if(!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Could not open file:" << file.fileName();
        return;
    }
    QTextStream stream(&file);

    // build and write string
    stream << "#!/bin/bash\n\n";
    stream << "#set up slit location\n";
    stream << "sed -i 's/^session.screen0.slit.placement:.*/session.screen0.slit.placement: " + slit_location + "/' $HOME/.fluxbox/init\n\n";
    stream << "fluxbox-remote restart\n\n";
    stream << "#commands for dock launchers\n";

    for (int i = 0; i < apps.size(); ++i) {
        QString command = (apps.at(i).at(0).endsWith(".desktop")) ? "--desktop-file " + apps.at(i).at(0) : "--command " + apps.at(i).at(1) + " --icon " + apps.at(i).at(2);
        stream << "wmalauncher " + command + " --background-color " + apps.at(i).at(4) + " --border-color " +
                  apps.at(i).at(5) + " --window-size " + apps.at(i).at(3).section("x", 0, 0) + " -x & sleep 0.1\n";
    }
    file.close();
    chmod(file_name.toUtf8(), 00744);

    if (dock_name.isEmpty()) dock_name = inputDockName();
    if (dock_name.isEmpty()) dock_name = QFileInfo(file).baseName();

    if (!isDockInMenu(file.fileName())) {
        addDockToMenu(file.fileName());
    }

    QMessageBox::information(this, tr("Dock saved"), tr("The dock has been saved.\n\n"
                                                        "To edit the newly created dock please select 'Edit an existing dock'."));

    cmd.run("pkill wmalauncher;" + file.fileName() + "&", true);

    index = 0;
    apps.clear();
    list_icons.clear();
    resetAdd();
    ui->buttonSave->setEnabled(false);
    ui->buttonDelete->setEnabled(false);
    setup();
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
    QString url = "https://mxlinux.org/wiki/help-files/help-mx-dockmaker/";
    displayDoc(url, tr("%1 Help").arg(this->windowTitle()), false);
}


void MainWindow::on_comboSize_currentIndexChanged(const QString)
{
    itemChanged();
}

void MainWindow::on_comboBgColor_currentIndexChanged(const QString)
{
    itemChanged();
}

void MainWindow::on_comboBorderColor_currentIndexChanged(const QString)
{
    itemChanged();
}


void MainWindow::on_buttonNext_clicked()
{
    blockComboSignals(true);
    ui->buttonSave->setEnabled(changed);
    ui->buttonNext->setEnabled(false);

    updateAppList(index++);
    ui->buttonDelete->setEnabled(true);
    ui->buttonPrev->setEnabled(true);
    if (index < apps.size()) {
        showApp(index);
        if (index == apps.size() - 1) ui->buttonNext->setEnabled(true);
    } else {
        resetAdd();
    }

    ui->buttonPrev->setEnabled(true);
    blockComboSignals(false);
}

void MainWindow::on_buttonDelete_clicked()
{
    if (!apps.isEmpty()) {
        blockComboSignals(true);
        delete ui->groupPreview->layout()->itemAt(index)->widget();
        list_icons.removeAt(index);
        apps.removeAt(index);
        blockComboSignals(false);
    }
    if (apps.isEmpty()) {
        index = 0;
        ui->buttonDelete->setEnabled(false);
        ui->buttonSave->setEnabled(false);
        ui->buttonPrev->setEnabled(false);
        resetAdd();
    } else if (index == 0) {
        ui->buttonPrev->setEnabled(false);
        showApp(index);
    } else {
        ui->buttonSave->setEnabled(true);
        showApp(--index);
    }
    changed = true;
}

void MainWindow::resetAdd()
{
    ui->buttonSelectApp->setText(tr("Select..."));
    ui->radioDesktop->click();
    ui->radioDesktop->toggled(true);

    blockComboSignals(true);
    ui->comboSize->setCurrentIndex(ui->comboSize->findText("48x48"));
    ui->comboBgColor->setCurrentIndex(ui->comboBgColor->findText(tr("black")));
    ui->comboBorderColor->setCurrentIndex(ui->comboBorderColor->findText(tr("white")));
    blockComboSignals(false);

    ui->buttonNext->setIcon(QIcon::fromTheme("go-jump"));
    ui->buttonNext->setText(tr("Add application"));
    ui->buttonNext->setEnabled(false);
    ui->buttonDelete->setEnabled(false);
    ui->buttonSelectIcon->setToolTip(QString());
    ui->buttonSelectIcon->setStyleSheet("text-align: left;");
}

void MainWindow::showApp(int idx)
{
    if (apps.at(idx).at(0).endsWith(".desktop")) {
        ui->radioDesktop->click();
        ui->radioDesktop->toggled(true);
        ui->buttonSelectIcon->setToolTip(QString());
        ui->buttonSelectIcon->setStyleSheet("text-align: left;");
    } else {
        ui->radioCommand->click();
        ui->radioDesktop->toggled(false);
        ui->lineEditCommand->setText(apps.at(idx).at(1));
        ui->buttonSelectIcon->setText(apps.at(idx).at(2));
        ui->buttonSelectIcon->setToolTip(apps.at(idx).at(2));
        ui->buttonSelectIcon->setStyleSheet("text-align: right;");
    }

    ui->buttonSelectApp->setText(apps.at(idx).at(0));

    blockComboSignals(true);
    ui->comboSize->setCurrentIndex(ui->comboSize->findText(apps.at(idx).at(3)));
    ui->comboBgColor->setCurrentIndex(ui->comboBgColor->findText(apps.at(idx).at(4)));
    ui->comboBorderColor->setCurrentIndex(ui->comboBorderColor->findText(apps.at(idx).at(5)));
    blockComboSignals(false);

    if (idx >= apps.size() - 1) {
        ui->buttonNext->setIcon(QIcon::fromTheme("go-jump"));
        ui->buttonNext->setText(tr("Add application"));
    } else {
        ui->buttonNext->setIcon(QIcon::fromTheme("go-next"));
        ui->buttonNext->setText(tr("Next"));
    }

    ui->buttonNext->setEnabled(true);
    ui->buttonPrev->setEnabled(true);
    if (idx == 0) ui->buttonPrev->setEnabled(false);

    list_icons.at(idx)->setStyleSheet(list_icons.at(idx)->styleSheet() + "border-width: 10px;");
    ui->buttonDelete->setEnabled(true);
}


void MainWindow::on_buttonSelectApp_clicked()
{
    QString selected = QFileDialog::getOpenFileName(nullptr, tr("Select .desktop file"), "/usr/share/applications", tr("Desktop Files (*.desktop)"));
    QString file = QFileInfo(selected).fileName();
    if (!file.isEmpty()) {
        file_name = file;
        ui->buttonSelectApp->setText(file);
        ui->buttonNext->setEnabled(true);
        itemChanged();
    }
}

void MainWindow::editDock()
{
    this->hide();
    QString selected_dock = QFileDialog::getOpenFileName(nullptr, tr("Select a dock file"), QDir::homePath() + "/.fluxbox/scripts");
    if (!QFileInfo::exists(selected_dock)) {
        QMessageBox::warning(nullptr, tr("No file selected"), tr("You haven't selected any dock file to edit.\nCreating a new dock instead."));
        setup();
        return;
    }
    QFile file(selected_dock);
    if(!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Could not open file:" << file.fileName();
        QMessageBox::warning(nullptr, tr("Could not open file"), tr("Could not open selected file.\nCreating a new dock instead."));
        newDock();
        return;
    }
    file_name = file.fileName();
    dock_name = getDockName(file_name);


    parseFile(file);
    file.close();
    ui->labelUsage->setText(tr("1. Edit applications one at a time using the Back and Next buttons\n"
                               "2. Add or delete applications as you like\n"
                               "3. When finished click Save"));
    this->show();
}

void MainWindow::newDock()
{
    dock_name = inputDockName();
    this->show();
    apps.clear();
    list_icons.clear();

    resetAdd();
    ui->buttonSave->setEnabled(false);
    ui->buttonDelete->setEnabled(false);
}

void MainWindow::on_buttonPrev_clicked()
{
    ui->buttonNext->setEnabled(false);
    ui->buttonPrev->setEnabled(false);
    ui->buttonSave->setEnabled(changed);
    if (index < apps.size() || ui->buttonNext->isEnabled()) { // when done editting
        updateAppList(index);
    }
    showApp(--index);
}

void MainWindow::on_radioDesktop_toggled(bool checked)
{
    ui->lineEditCommand->clear();
    ui->buttonSelectApp->setEnabled(checked);
    ui->lineEditCommand->setEnabled(!checked);
    ui->buttonSelectIcon->setEnabled(!checked);
    if (!checked) ui->buttonSelectApp->setText(tr("Select..."));
    if (checked) ui->buttonSelectIcon->setText(tr("Select icon..."));
}

void MainWindow::on_buttonSelectIcon_clicked()
{
    ui->buttonSave->setDisabled(ui->buttonNext->isEnabled());
    QString selected = QFileDialog::getOpenFileName(nullptr, tr("Select icon"), "/usr/share/icons/Moka", tr("Icons (*.png *.jpg *.bmp *.xpm *.svg)"));
    QString file = QFileInfo(selected).fileName();
    if (!file.isEmpty()) {
        ui->buttonSelectIcon->setText(selected);
        ui->buttonSelectIcon->setToolTip(selected);
        ui->buttonSelectIcon->setStyleSheet("text-align: right;");
        updateAppList(index);
    }
   ifNotDoneDisableButtons();
}

void MainWindow::on_lineEditCommand_textEdited(const QString)
{
    ui->buttonSave->setDisabled(ui->buttonNext->isEnabled());
    if(ui->buttonSelectIcon->text() != tr("Select icon...")) {
        ui->buttonNext->setEnabled(true);
        changed = true;
    } else {
        ui->buttonNext->setEnabled(false);
        return;
    }
    ifNotDoneDisableButtons();
}

