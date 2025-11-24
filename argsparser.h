#pragma once
#include <QString>
#include <QStringList>

class ArgsParser {
public:
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
        return args;}};
