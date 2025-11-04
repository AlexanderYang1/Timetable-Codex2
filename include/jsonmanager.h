#pragma once

#include "models.h"

#include <QObject>

class JsonManager : public QObject
{
    Q_OBJECT
public:
    explicit JsonManager(QObject *parent = nullptr);

    void ensureDataFiles();

    QVector<Activity> loadActivities() const;
    void saveActivities(const QVector<Activity> &activities) const;

    QVector<Task> loadTasks() const;
    void saveTasks(const QVector<Task> &tasks) const;

    SettingsData loadSettings() const;
    void saveSettings(const SettingsData &settings) const;

    SchoolPeriodsData loadSchoolPeriods() const;

    QVector<TimetablePeriod> upcomingPeriods(const SettingsData &settings, const QDateTime &from, const QDateTime &to) const;

    QString dataDirectory() const;

signals:
    void dataDirectoryChanged(const QString &path);

private:
    QString resolveDataDirectory() const;
    QString ensureFile(const QString &fileName, const QString &defaultResource) const;
    Activity activityFromJson(const QJsonObject &obj) const;
    QJsonObject activityToJson(const Activity &activity) const;
    Subtask subtaskFromJson(const QJsonObject &obj) const;
    QJsonObject subtaskToJson(const Subtask &subtask) const;
    Task taskFromJson(const QJsonObject &obj) const;
    QJsonObject taskToJson(const Task &task) const;
    SchoolPeriodsData parseSchoolPeriods(const QJsonDocument &doc) const;
};
