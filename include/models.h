#pragma once

#include <QColor>
#include <QDateTime>
#include <QMap>
#include <QString>
#include <QVector>

struct Activity
{
    QString id;
    QString title;
    QString description;
    QDateTime startTime;
    QDateTime endTime;
    QColor color;
};

struct Subtask
{
    QString id;
    QString title;
    QString description;
    QDateTime dueTime;
    double weighting = 1.0;
    bool completed = false;
};

struct Task
{
    QString id;
    QString title;
    QString description;
    QDateTime startTime;
    QDateTime endTime;
    QVector<Subtask> subtasks;
};

struct SettingsData
{
    QString currentWeek = "A";
    int yearLevel = 10;
};

struct SubjectDefinition
{
    QString name;
    QString teacher;
    QColor color;
};

struct PeriodTime
{
    QString label;
    QTime start;
    QTime end;
};

struct TimetableSubjectSlot
{
    QString periodKey;
    QString subjectName;
    QString room;
};

struct TimetablePeriod
{
    QString subjectName;
    QString room;
    QString teacher;
    QColor color;
    QDateTime startTime;
    QDateTime endTime;
    QString periodKey;
    bool isSpecial = false;
};

struct TimetableTemplate
{
    QString name;
    QVector<PeriodTime> periods;
};

struct DaySchedule
{
    QString name;
    QString templateName;
    QVector<TimetableSubjectSlot> slots;
};

struct WeekSchedule
{
    QString name;
    QMap<QString, DaySchedule> days; // Monday-Friday
};

struct SchoolPeriodsData
{
    QMap<QString, SubjectDefinition> subjects;
    QMap<QString, TimetableTemplate> templates;
    QMap<QString, WeekSchedule> weeks; // A/B
};

struct DonutArc
{
    QDateTime startTime;
    QDateTime endTime;
    QColor color;
    QString label;
    QString category; // e.g., "Activity" or "Period"
};
