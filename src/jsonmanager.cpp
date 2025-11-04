#include "jsonmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUuid>

namespace
{
constexpr char kActivitiesFile[] = "activities.json";
constexpr char kTasksFile[] = "tasks.json";
constexpr char kSettingsFile[] = "settings.json";
constexpr char kSchoolPeriodsFile[] = "SchoolPeriods.json";

constexpr char kActivitiesDefault[] = ":/defaults/activities.json";
constexpr char kTasksDefault[] = ":/defaults/tasks.json";
constexpr char kSettingsDefault[] = ":/defaults/settings.json";
constexpr char kSchoolPeriodsDefault[] = ":/defaults/SchoolPeriods.json";

QDateTime parseIsoDateTime(const QString &value)
{
    return QDateTime::fromString(value, Qt::ISODate);
}

QString toIsoString(const QDateTime &dateTime)
{
    return dateTime.toString(Qt::ISODate);
}
}

JsonManager::JsonManager(QObject *parent)
    : QObject(parent)
{
}

void JsonManager::ensureDataFiles()
{
    ensureFile(kActivitiesFile, kActivitiesDefault);
    ensureFile(kTasksFile, kTasksDefault);
    ensureFile(kSettingsFile, kSettingsDefault);
    ensureFile(kSchoolPeriodsFile, kSchoolPeriodsDefault);
}

QString JsonManager::dataDirectory() const
{
    return resolveDataDirectory();
}

QString JsonManager::resolveDataDirectory() const
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty())
    {
        baseDir = QCoreApplication::applicationDirPath() + "/data";
    }

    QDir dir(baseDir);
    if (!dir.exists())
    {
        dir.mkpath(".");
    }
    return dir.absolutePath();
}

QString JsonManager::ensureFile(const QString &fileName, const QString &defaultResource) const
{
    const QString path = resolveDataDirectory() + QDir::separator() + fileName;
    QFile target(path);
    if (!target.exists())
    {
        QFile resource(defaultResource);
        if (resource.exists() && resource.open(QIODevice::ReadOnly))
        {
            if (target.open(QIODevice::WriteOnly))
            {
                target.write(resource.readAll());
                target.close();
            }
            resource.close();
        }
        else
        {
            if (target.open(QIODevice::WriteOnly))
            {
                target.write("{}\n");
                target.close();
            }
        }
    }
    return path;
}

QVector<Activity> JsonManager::loadActivities() const
{
    const QString path = ensureFile(kActivitiesFile, kActivitiesDefault);
    QFile file(path);
    QVector<Activity> items;
    if (!file.open(QIODevice::ReadOnly))
    {
        return items;
    }

    const auto doc = QJsonDocument::fromJson(file.readAll());
    const auto root = doc.object();
    const auto array = root.value("activities").toArray();
    for (const QJsonValue &value : array)
    {
        items.append(activityFromJson(value.toObject()));
    }
    return items;
}

void JsonManager::saveActivities(const QVector<Activity> &activities) const
{
    const QString path = ensureFile(kActivitiesFile, kActivitiesDefault);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return;
    }

    QJsonArray array;
    for (const auto &activity : activities)
    {
        array.append(activityToJson(activity));
    }

    QJsonObject root;
    root.insert("activities", array);
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

QVector<Task> JsonManager::loadTasks() const
{
    const QString path = ensureFile(kTasksFile, kTasksDefault);
    QFile file(path);
    QVector<Task> items;
    if (!file.open(QIODevice::ReadOnly))
    {
        return items;
    }

    const auto doc = QJsonDocument::fromJson(file.readAll());
    const auto root = doc.object();
    const auto array = root.value("tasks").toArray();
    for (const QJsonValue &value : array)
    {
        items.append(taskFromJson(value.toObject()));
    }
    return items;
}

void JsonManager::saveTasks(const QVector<Task> &tasks) const
{
    const QString path = ensureFile(kTasksFile, kTasksDefault);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return;
    }

    QJsonArray array;
    for (const auto &task : tasks)
    {
        array.append(taskToJson(task));
    }

    QJsonObject root;
    root.insert("tasks", array);
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

SettingsData JsonManager::loadSettings() const
{
    const QString path = ensureFile(kSettingsFile, kSettingsDefault);
    QFile file(path);
    SettingsData settings;
    if (!file.open(QIODevice::ReadOnly))
    {
        return settings;
    }

    const auto doc = QJsonDocument::fromJson(file.readAll());
    const auto root = doc.object();
    settings.currentWeek = root.value("current_week").toString(settings.currentWeek);
    settings.yearLevel = root.value("year_level").toInt(settings.yearLevel);
    return settings;
}

void JsonManager::saveSettings(const SettingsData &settings) const
{
    const QString path = ensureFile(kSettingsFile, kSettingsDefault);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return;
    }

    QJsonObject root;
    root.insert("current_week", settings.currentWeek);
    root.insert("year_level", settings.yearLevel);
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

SchoolPeriodsData JsonManager::loadSchoolPeriods() const
{
    const QString path = ensureFile(kSchoolPeriodsFile, kSchoolPeriodsDefault);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        return SchoolPeriodsData{};
    }

    const auto doc = QJsonDocument::fromJson(file.readAll());
    return parseSchoolPeriods(doc);
}

QVector<TimetablePeriod> JsonManager::upcomingPeriods(const SettingsData &settings, const QDateTime &from, const QDateTime &to) const
{
    QVector<TimetablePeriod> result;
    const auto data = loadSchoolPeriods();

    QDateTime cursor = from;
    while (cursor < to)
    {
        const QString weekKey = settings.currentWeek.toUpper();
        if (!data.weeks.contains(weekKey))
        {
            break;
        }

        const WeekSchedule &week = data.weeks.value(weekKey);
        const QString dayName = cursor.date().toString("dddd");
        if (!week.days.contains(dayName))
        {
            cursor = cursor.addDays(1).setTime(QTime(0, 0));
            continue;
        }

        const DaySchedule &day = week.days.value(dayName);
        if (!data.templates.contains(day.templateName))
        {
            cursor = cursor.addDays(1).setTime(QTime(0, 0));
            continue;
        }

        const TimetableTemplate &templ = data.templates.value(day.templateName);
        QMap<QString, TimetableSubjectSlot> slotLookup;
        for (const auto &slot : day.slots)
        {
            slotLookup.insert(slot.periodKey, slot);
        }

        for (const auto &period : templ.periods)
        {
            const QDateTime start(cursor.date(), period.start);
            const QDateTime end(cursor.date(), period.end);
            if (end <= from || start >= to)
            {
                continue;
            }

            TimetablePeriod ttPeriod;
            ttPeriod.startTime = start;
            ttPeriod.endTime = end;
            ttPeriod.periodKey = period.label;
            ttPeriod.isSpecial = !slotLookup.contains(period.label);

            if (slotLookup.contains(period.label))
            {
                const auto slot = slotLookup.value(period.label);
                ttPeriod.subjectName = slot.subjectName;
                ttPeriod.room = slot.room;
                const auto subject = data.subjects.value(slot.subjectName);
                ttPeriod.teacher = subject.teacher;
                ttPeriod.color = subject.color;
            }
            else
            {
                ttPeriod.subjectName = period.label.toUpper();
                ttPeriod.room = {};
                ttPeriod.teacher = {};
                ttPeriod.color = QColor(224, 224, 224);
            }

            result.append(ttPeriod);
        }

        cursor = cursor.addDays(1).setTime(QTime(0, 0));
    }

    std::sort(result.begin(), result.end(), [](const TimetablePeriod &a, const TimetablePeriod &b) {
        return a.startTime < b.startTime;
    });

    return result;
}

Activity JsonManager::activityFromJson(const QJsonObject &obj) const
{
    Activity activity;
    activity.id = obj.value("id").toString(QUuid::createUuid().toString(QUuid::WithoutBraces));
    activity.title = obj.value("title").toString();
    activity.description = obj.value("description").toString();
    activity.startTime = parseIsoDateTime(obj.value("start_time").toString());
    activity.endTime = parseIsoDateTime(obj.value("end_time").toString());
    activity.color = QColor(obj.value("color").toString("#4ECDC4"));
    return activity;
}

QJsonObject JsonManager::activityToJson(const Activity &activity) const
{
    QJsonObject obj;
    obj.insert("id", activity.id);
    obj.insert("title", activity.title);
    obj.insert("description", activity.description);
    obj.insert("start_time", toIsoString(activity.startTime));
    obj.insert("end_time", toIsoString(activity.endTime));
    obj.insert("color", activity.color.name(QColor::HexRgb));
    return obj;
}

Subtask JsonManager::subtaskFromJson(const QJsonObject &obj) const
{
    Subtask subtask;
    subtask.id = obj.value("id").toString(QUuid::createUuid().toString(QUuid::WithoutBraces));
    subtask.title = obj.value("title").toString();
    subtask.description = obj.value("description").toString();
    subtask.dueTime = parseIsoDateTime(obj.value("due_time").toString());
    subtask.weighting = obj.value("weighting").toDouble(1.0);
    subtask.completed = obj.value("completed").toBool(false);
    return subtask;
}

QJsonObject JsonManager::subtaskToJson(const Subtask &subtask) const
{
    QJsonObject obj;
    obj.insert("id", subtask.id);
    obj.insert("title", subtask.title);
    obj.insert("description", subtask.description);
    obj.insert("due_time", toIsoString(subtask.dueTime));
    obj.insert("weighting", subtask.weighting);
    obj.insert("completed", subtask.completed);
    return obj;
}

Task JsonManager::taskFromJson(const QJsonObject &obj) const
{
    Task task;
    task.id = obj.value("id").toString(QUuid::createUuid().toString(QUuid::WithoutBraces));
    task.title = obj.value("title").toString();
    task.description = obj.value("description").toString();
    task.startTime = parseIsoDateTime(obj.value("start_time").toString());
    task.endTime = parseIsoDateTime(obj.value("end_time").toString());

    const auto subtasksArray = obj.value("subtasks").toArray();
    for (const QJsonValue &value : subtasksArray)
    {
        task.subtasks.append(subtaskFromJson(value.toObject()));
    }
    return task;
}

QJsonObject JsonManager::taskToJson(const Task &task) const
{
    QJsonObject obj;
    obj.insert("id", task.id);
    obj.insert("title", task.title);
    obj.insert("description", task.description);
    obj.insert("start_time", toIsoString(task.startTime));
    obj.insert("end_time", toIsoString(task.endTime));

    QJsonArray subtasksArray;
    for (const auto &subtask : task.subtasks)
    {
        subtasksArray.append(subtaskToJson(subtask));
    }
    obj.insert("subtasks", subtasksArray);
    return obj;
}

SchoolPeriodsData JsonManager::parseSchoolPeriods(const QJsonDocument &doc) const
{
    SchoolPeriodsData data;
    const auto root = doc.object();

    const auto subjectsObj = root.value("subjects").toObject();
    for (auto it = subjectsObj.begin(); it != subjectsObj.end(); ++it)
    {
        SubjectDefinition definition;
        definition.name = it.key();
        const auto subjectObj = it.value().toObject();
        definition.teacher = subjectObj.value("teacher").toString();
        definition.color = QColor(subjectObj.value("color").toString("#E0E0E0"));
        data.subjects.insert(definition.name, definition);
    }

    const auto templatesObj = root.value("period_times").toObject();
    for (auto it = templatesObj.begin(); it != templatesObj.end(); ++it)
    {
        TimetableTemplate templ;
        templ.name = it.key();
        const auto templObj = it.value().toObject();
        QVector<PeriodTime> periods;
        for (auto pit = templObj.begin(); pit != templObj.end(); ++pit)
        {
            PeriodTime period;
            period.label = pit.key();
            const auto times = pit.value().toObject();
            period.start = QTime::fromString(times.value("start_time").toString(), "hh:mm");
            period.end = QTime::fromString(times.value("end_time").toString(), "hh:mm");
            periods.append(period);
        }

        std::sort(periods.begin(), periods.end(), [](const PeriodTime &a, const PeriodTime &b) {
            if (a.start == b.start)
            {
                return a.label < b.label;
            }
            return a.start < b.start;
        });

        templ.periods = periods;
        data.templates.insert(templ.name, templ);
    }

    const auto weeksObj = root.value("schedule").toObject();
    for (auto wit = weeksObj.begin(); wit != weeksObj.end(); ++wit)
    {
        WeekSchedule week;
        week.name = wit.key();
        const auto weekObj = wit.value().toObject();
        for (auto dit = weekObj.begin(); dit != weekObj.end(); ++dit)
        {
            DaySchedule day;
            day.name = dit.key();
            const auto dayObj = dit.value().toObject();
            day.templateName = dayObj.value("period_template").toString();
            const auto subjects = dayObj.value("subjects").toObject();
            for (auto sit = subjects.begin(); sit != subjects.end(); ++sit)
            {
                TimetableSubjectSlot slot;
                slot.periodKey = sit.key();
                const auto slotObj = sit.value().toObject();
                slot.subjectName = slotObj.value("subject").toString();
                slot.room = slotObj.value("room").toString();
                day.slots.append(slot);
            }
            week.days.insert(day.name, day);
        }
        data.weeks.insert(week.name.toUpper(), week);
    }

    return data;
}
