#pragma once
#include <QString>
#include <QStringList>
#include <QMap>

class ArgsParser {
public:
    static QMap<QString, QString> s_options;
    static QStringList parse(const QString &command) {
        QStringList args;
        QString current;
        bool inQuote = false;
        bool inSingleQuote = false;
        for (int i = 0; i < command.length(); ++i) {
            QChar c = command[i];
            if (c == '"' && !inSingleQuote) {
                inQuote = !inQuote;
            } else if (c == '\'' && !inQuote) {
                inSingleQuote = !inSingleQuote;
            } else if (c.isSpace() && !inQuote && !inSingleQuote) {
                if (!current.isEmpty()) {
                    args.append(current);
                    current.clear();}
            } else {
                current.append(c);}}
        if (!current.isEmpty()) args.append(current);
        return args;}
    static void parse(const QStringList &arguments) {
        s_options.clear();
        for (int i = 1; i < arguments.size(); ++i) {
            QString arg = arguments.at(i);
            if (arg.startsWith("-")) {
                QString key;
                QString value;
                if (arg.contains('=')) {
                    QStringList parts = arg.mid(1).split('=', Qt::SkipEmptyParts);
                    key = parts.at(0).trimmed().remove('-');
                    if (parts.size() > 1) {
                        value = parts.at(1).trimmed();
                    } else {
                        value = "true";}
                } else {
                    key = arg.mid(1).remove('-');
                    if (i + 1 < arguments.size() && !arguments.at(i + 1).startsWith("-")) {
                        value = arguments.at(i + 1);
                        i++;
                    } else {
                        value = "true";}}
                if (!key.isEmpty()) {
                    s_options.insert(key, value.remove('"').remove('\''));}}}}
    static bool isDefined(const QString &key) {
        return s_options.contains(key); }
    static QString get(const QString &key) {
        return s_options.value(key);}};
