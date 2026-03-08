/**********************************************************************
 *  dockfileparser_test.cpp
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

#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "dockconfiguration.h"
#include "dockfileparser.h"

class DockFileParserTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesMultipleEntriesAndDefaults();
    void parsesCommandsWithQuotedArguments();
    void parsesQuotedDesktopAndIconValues();
    void parsesCommandsWithDashXFlag();
    void roundTripCommandWithDashX();
    void parsesWmalauncherDashXBeforeCommand();
    void extractsDockNameWithoutTruncation();
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
    QCOMPARE(first.command, QStringLiteral("echo hello world"));

    const DockIconInfo second = configuration.getApplication(1);
    QCOMPARE(second.command, QStringLiteral("/usr/bin/my_app arg one arg two with space"));
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
