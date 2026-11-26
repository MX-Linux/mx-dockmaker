/**********************************************************************
 *  dockfileparser_test.cpp
 **********************************************************************
 * Copyright (C) 2020-2026 MX Authors
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

#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QScopeGuard>

#include "dockfilemanager.h"

#include "dockconfiguration.h"
#include "dockfileparser.h"

class DockFileParserTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesMultipleEntriesAndDefaults();
    void preservesIconSizesAbove255();
    void parsesCommandsWithQuotedArguments();
    void savesAndReloadsQuotedArguments();
    void parsesQuotedDesktopAndIconValues();
    void parsesCommandsWithDashXFlag();
    void roundTripCommandWithDashX();
    void parsesWmalauncherDashXBeforeCommand();
    void extractsDockNameWithoutTruncation();
    void matchesStockMenuEntryByPath();
    void writesQuotedMenuEntry();
    void deletesDockWithMissingMenu();
    void extractsSlitLocationFromPlacementCommand();
    void ignoresUnrelatedLocationTokensInContent();
};

void DockFileParserTest::parsesMultipleEntriesAndDefaults()
{
    const QString content = QStringLiteral(
        "wmalauncher --desktop-file app.desktop --background-color black --hover-background-color black "
        "--border-color white --hover-border-color white --window-size 64\n"
        "wmalauncher --command echo hi --icon /usr/share/icons/hi.png --background-color black "
        "--hover-background-color black --border-color white --hover-border-color white\n");

    DockConfiguration configuration;
    DockFileParser parser;

    QVERIFY(parser.parseContent(content, configuration));
    QCOMPARE(configuration.getApplicationCount(), 2);

    const DockIconInfo first = configuration.getApplication(0);
    QCOMPARE(first.iconSize(), QSize(64, 64));
    QCOMPARE(first.size, QStringLiteral("64x64"));

    const DockIconInfo second = configuration.getApplication(1);
    QCOMPARE(second.iconSize(), QSize(48, 48)); // falls back to default size when not provided
    QCOMPARE(second.size, QStringLiteral("48x48"));
    QVERIFY(second.isValid());
}

void DockFileParserTest::preservesIconSizesAbove255()
{
    const QString content = QStringLiteral("wmalauncher --command echo test --window-size 999\n");

    DockConfiguration configuration;
    DockFileParser parser;

    QVERIFY(parser.parseContent(content, configuration));
    QCOMPARE(configuration.getApplication(0).iconSize(), QSize(999, 999));
}

void DockFileParserTest::parsesCommandsWithQuotedArguments()
{
    const QString content = QStringLiteral(
        "wmalauncher --command echo \"hello world\" --icon /usr/share/icons/hello.png --background-color black "
        "--hover-background-color black --border-color white --hover-border-color white\n"
        "wmalauncher --command /usr/bin/my_app \"arg one\" 'arg two with space' --icon /usr/share/icons/app.png "
        "--background-color black --hover-background-color black --border-color white --hover-border-color white\n");

    DockConfiguration configuration;
    DockFileParser parser;

    QVERIFY(parser.parseContent(content, configuration));
    QCOMPARE(configuration.getApplicationCount(), 2);

    const DockIconInfo first = configuration.getApplication(0);
    QCOMPARE(first.command, QStringLiteral("echo \"hello world\""));

    const DockIconInfo second = configuration.getApplication(1);
    QCOMPARE(second.command, QStringLiteral("/usr/bin/my_app \"arg one\" 'arg two with space'"));
}

void DockFileParserTest::parsesQuotedDesktopAndIconValues()
{
    const QString content = QStringLiteral(
        "wmalauncher --desktop-file \"my app.desktop\" --icon '/usr/share/icons/my icon.png' "
        "--background-color black --hover-background-color black --border-color white --hover-border-color white\n");

    DockConfiguration configuration;
    DockFileParser parser;

    QVERIFY(parser.parseContent(content, configuration));
    QCOMPARE(configuration.getApplicationCount(), 1);

    const DockIconInfo app = configuration.getApplication(0);
    QCOMPARE(app.appName, QStringLiteral("my app.desktop"));
    QCOMPARE(app.customIcon, QStringLiteral("/usr/share/icons/my icon.png"));
}

QTEST_GUILESS_MAIN(DockFileParserTest)
#include "dockfileparser_test.moc"

void DockFileParserTest::parsesCommandsWithDashXFlag()
{
    // Bug test: Commands with -x flags should be fully preserved
    // Testing unquoted command (wmalauncher does not handle quoted commands)
    const QString content = QStringLiteral(
        "wmalauncher --command xfce4-terminal --hold -x bc --icon terminal.png "
        "--background-color black --hover-background-color black --border-color white --hover-border-color white\n");

    DockConfiguration configuration;
    DockFileParser parser;

    QVERIFY(parser.parseContent(content, configuration));
    QCOMPARE(configuration.getApplicationCount(), 1);

    const DockIconInfo app = configuration.getApplication(0);
    QCOMPARE(app.command, QStringLiteral("xfce4-terminal --hold -x bc"));
}

void DockFileParserTest::roundTripCommandWithDashX()
{
    // End-to-end test: Simulates what DockFileManager would generate
    // for a command with -x flag, then parses it back
    const QString generatedContent = QStringLiteral(
        "#!/bin/bash\n\n"
        "pkill -x wmalauncher\n\n"
        "#commands for dock launchers\n"
        "wmalauncher --command xfce4-terminal --hold -x bc --icon 'terminal.png' "
        "--background-color \"#000000\" --hover-background-color \"#000000\" "
        "--border-color \"#ffffff\" --hover-border-color \"#ffffff\" "
        "--window-size 48& sleep 0.1\n");

    DockConfiguration configuration;
    DockFileParser parser;

    QVERIFY(parser.parseContent(generatedContent, configuration));
    QCOMPARE(configuration.getApplicationCount(), 1);

    const DockIconInfo app = configuration.getApplication(0);
    QCOMPARE(app.command, QStringLiteral("xfce4-terminal --hold -x bc"));
    QCOMPARE(app.customIcon, QStringLiteral("terminal.png"));
}

void DockFileParserTest::parsesWmalauncherDashXBeforeCommand()
{
    const QString content = QStringLiteral(
        "wmalauncher -x --command xfce4-terminal --hold -x bc --icon terminal.png "
        "--background-color black --hover-background-color black --border-color white --hover-border-color white\n");

    DockConfiguration configuration;
    DockFileParser parser;

    QVERIFY(parser.parseContent(content, configuration));
    QCOMPARE(configuration.getApplicationCount(), 1);

    const DockIconInfo app = configuration.getApplication(0);
    QCOMPARE(app.command, QStringLiteral("xfce4-terminal --hold -x bc"));
    QCOMPARE(app.customIcon, QStringLiteral("terminal.png"));
}

void DockFileParserTest::extractsDockNameWithoutTruncation()
{
    QTemporaryDir tempHome;
    QVERIFY(tempHome.isValid());

    const QString submenuDir = tempHome.path() + QStringLiteral("/.fluxbox/submenus");
    QVERIFY(QDir().mkpath(submenuDir));

    QFile submenuFile(submenuDir + QStringLiteral("/appearance"));
    QVERIFY(submenuFile.open(QFile::WriteOnly | QFile::Text));
    submenuFile.write("[submenu] (Docks)\n\t\t\t[exec] (My Dock) {/tmp/mydock.mxdk}\n");
    submenuFile.close();

    const QByteArray originalHome = qgetenv("HOME");
    qputenv("HOME", tempHome.path().toUtf8());

    QCOMPARE(DockFileParser::extractDockName(QStringLiteral("/tmp/mydock.mxdk")), QStringLiteral("My Dock"));

    if (originalHome.isEmpty()) {
        qunsetenv("HOME");
    } else {
        qputenv("HOME", originalHome);
    }
}

void DockFileParserTest::extractsSlitLocationFromPlacementCommand()
{
    DockFileParser parser;

    const QString content = QStringLiteral(
        "#set up slit location\n"
        "sed -i 's/^session.screen0.slit.placement:.*/session.screen0.slit.placement: BottomRight/' $HOME/.fluxbox/init\n"
        "wmalauncher --command echo test --icon terminal.png --background-color black "
        "--hover-background-color black --border-color white --hover-border-color white\n");

    QCOMPARE(parser.extractSlitLocation(content), QStringLiteral("BottomRight"));
}

void DockFileParserTest::ignoresUnrelatedLocationTokensInContent()
{
    DockFileParser parser;

    const QString content = QStringLiteral(
        "wmalauncher --command echo BottomRight --icon terminal.png "
        "--tooltip-text 'TopLeft marker' --background-color black "
        "--hover-background-color black --border-color white --hover-border-color white\n");

    QVERIFY(parser.extractSlitLocation(content).isEmpty());
}

void DockFileParserTest::matchesStockMenuEntryByPath()
{
    const QString stock = QStringLiteral("    [exec] (Default) {~/.fluxbox/scripts/DefaultDock.mxdk}");
    const QString path = QDir::home().filePath(QStringLiteral(".fluxbox/scripts/DefaultDock.mxdk"));
    QCOMPARE(DockFileParser::menuEntryName(stock, path), QStringLiteral("Default"));
    QVERIFY(DockFileParser::menuEntryName(stock, path.chopped(5)).isEmpty());
    QVERIFY(DockFileParser::menuEntryName(stock, QStringLiteral("/another/DefaultDock.mxdk")).isEmpty());
    QCOMPARE(DockFileParser::menuEntryName(QStringLiteral("t\t\t[exec] (Saved) {%1}").arg(path), path),
             QStringLiteral("Saved"));
}

void DockFileParserTest::savesAndReloadsQuotedArguments()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QByteArray oldHome = qgetenv("HOME");
    const auto restoreHome = qScopeGuard([&] { oldHome.isNull() ? qunsetenv("HOME") : qputenv("HOME", oldHome); });
    qputenv("HOME", dir.path().toUtf8());

    DockIconInfo icon;
    icon.command = QStringLiteral("viewer \"O'Brien's Notes.pdf\" \"\" 'A & B' -c ~/bin/tool \"$HOME/My file\" '$HOME'");
    icon.customIcon = QStringLiteral("/icons/Reader's icon.png");
    icon.extraOptions = QStringLiteral("--tooltip-font \"Reader's Font/10\"");
    DockConfiguration original;
    QVERIFY(original.addApplication(icon) >= 0);

    const QString file = dir.filePath(QStringLiteral("dock.mxdk"));
    DockFileManager files;
    QVERIFY2(files.saveConfiguration(original, file), qPrintable(files.getLastError()));
    DockConfiguration loaded;
    QVERIFY2(files.loadConfiguration(file, loaded), qPrintable(files.getLastError()));
    QCOMPARE(loaded.getApplication(0).customIcon, icon.customIcon);
    QVERIFY2(files.saveConfiguration(loaded, file), qPrintable(files.getLastError()));

    QFile script(file);
    QVERIFY(script.open(QIODevice::ReadOnly));
    const QString line = QString::fromUtf8(script.readAll()).section(QStringLiteral("\nwmalauncher "), 1)
                             .section(QStringLiteral("& sleep"), 0, 0);
    QVERIFY(!line.isEmpty());
    // Ask sh for the argument vector without starting any dock processes.
    QProcess shell;
    shell.start(QStringLiteral("/bin/sh"),
                {QStringLiteral("-c"), QStringLiteral("set -- ") + line + QStringLiteral("\nprintf '%s\\n' \"$@\"")});
    QVERIFY(shell.waitForFinished());
    QCOMPARE(shell.exitCode(), 0);
    const auto args = shell.readAllStandardOutput().split('\n');
    QCOMPARE(args.mid(0, 11), QList<QByteArray>({"--command", "viewer", "O'Brien's Notes.pdf", "", "A & B", "-c",
                                                dir.filePath("bin/tool").toUtf8(), dir.filePath("My file").toUtf8(), "$HOME",
                                                "--icon", "/icons/Reader's icon.png"}));
    QVERIFY(args.contains("Reader's Font/10"));
}

void DockFileParserTest::writesQuotedMenuEntry()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QByteArray oldHome = qgetenv("HOME");
    const auto restoreHome = qScopeGuard([&] { oldHome.isNull() ? qunsetenv("HOME") : qputenv("HOME", oldHome); });
    qputenv("HOME", dir.path().toUtf8());
    QVERIFY(QDir().mkpath(dir.filePath(".fluxbox/submenus")));
    QFile menu(dir.filePath(".fluxbox/submenus/appearance"));
    QVERIFY(menu.open(QIODevice::WriteOnly));
    const QByteArray header("[submenu] (Docks)\r\n");
    menu.write(header);
    menu.close();

    DockFileManager files;
    const QString path = dir.filePath(QStringLiteral("Sam's {Music} Dock.mxdk"));
    const QString name = QStringLiteral("Music (work)");
    QVERIFY(files.addToMenu(path, name));
    QVERIFY(files.addToMenu(path, name));
    QVERIFY(files.isInMenu(path));
    QCOMPARE(DockFileParser::extractDockName(path), name);
    QVERIFY(menu.open(QIODevice::ReadOnly));
    const QByteArray contents = menu.readAll();
    menu.close();
    QCOMPARE(contents.count("[exec]"), 1);
    QVERIFY(contents.contains("Music (work\\))"));
    QVERIFY(contents.contains("{Music\\} Dock.mxdk'"));
    QVERIFY(files.removeFromMenu(path));
    QVERIFY(menu.open(QIODevice::ReadOnly));
    QCOMPARE(menu.readAll(), header);
}

void DockFileParserTest::deletesDockWithMissingMenu()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QByteArray oldHome = qgetenv("HOME");
    const auto restoreHome = qScopeGuard([&] { oldHome.isNull() ? qunsetenv("HOME") : qputenv("HOME", oldHome); });
    qputenv("HOME", dir.path().toUtf8());

    QFile dock(dir.filePath("dock.mxdk"));
    QVERIFY(dock.open(QIODevice::WriteOnly));
    dock.close();
    DockFileManager files;
    QVERIFY(files.deleteDockFile(dock.fileName()));
    QVERIFY(!dock.exists());

    // A real error reading an existing menu must still leave the dock alone.
    QVERIFY(QDir().mkpath(dir.filePath(".fluxbox/submenus/appearance")));
    QVERIFY(dock.open(QIODevice::WriteOnly));
    dock.close();
    QVERIFY(!files.deleteDockFile(dock.fileName()));
    QVERIFY(dock.exists());
}
