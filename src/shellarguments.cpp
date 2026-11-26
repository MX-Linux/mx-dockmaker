// SPDX-License-Identifier: GPL-3.0-or-later
#include "shellarguments.h"

#include <QRegularExpression>
#include <glib.h>

bool ShellArguments::split(const QString &text, QStringList &arguments, QString &error, QStringList *spellings)
{
    arguments.clear();
    if (spellings) {
        spellings->clear();
    }
    if (text.trimmed().isEmpty()) {
        return true;
    }
    gchar **argv = nullptr;
    GError *parseError = nullptr;
    if (!g_shell_parse_argv(text.toUtf8().constData(), nullptr, &argv, &parseError)) {
        error = QString::fromUtf8(parseError->message);
        g_error_free(parseError);
        return false;
    }
    // GLib returns the decoded argv. Keep the source words as well so quoted
    // literals and shell expansions (e.g. '$HOME' and "$HOME") remain distinct.
    static const QRegularExpression word(QStringLiteral(R"rx((?:'[^']*'|"(?:\\.|[^"\\])*"|\\.|[^ \t\n'"\\])+)rx"));
    static const QRegularExpression singleArgument(
        QStringLiteral(R"rx(\A(?:'[^']*'|"(?:\\.|[^"\\])*"|\\.|[^ \t\n'"\\;&|<>()])+\z)rx"));
    auto source = word.globalMatch(text);
    for (auto arg = argv; *arg; ++arg) {
        const QString value = QString::fromUtf8(*arg);
        arguments.append(value);
        if (spellings) {
            const QString raw = source.hasNext() ? source.next().captured() : QString();
            spellings->append(singleArgument.match(raw).hasMatch() ? raw : quote(value));
        }
    }
    g_strfreev(argv);
    return true;
}

QString ShellArguments::quote(const QString &argument)
{
    gchar *escaped = g_shell_quote(argument.toUtf8().constData());
    const QString result = QString::fromUtf8(escaped);
    g_free(escaped);
    return result;
}
