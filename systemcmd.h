#ifndef SYSTEMCMD_H
#define SYSTEMCMD_H

#include <QString>
#include <string>
#include "nlohmann/json.hpp"

struct SystemCmd {
    QString command;
    QString description;
    nlohmann::json toJson() const {
        return nlohmann::json{
            {"command", command.toStdString()},
            {"description", description.toStdString()}};}
    static SystemCmd fromJson(const nlohmann::json &j) {
        SystemCmd cmd;
        if (j.contains("command")) {
            cmd.command = QString::fromStdString(j.at("command").get<std::string>());}
        if (j.contains("description")) {
            cmd.description = QString::fromStdString(j.at("description").get<std::string>());}
        return cmd;}};

#endif // SYSTEMCMD_H
