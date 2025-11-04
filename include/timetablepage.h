#pragma once

#include "jsonmanager.h"
#include "models.h"

#include <QButtonGroup>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QVector>
#include <QWidget>

class TimetableCard : public QFrame
{
    Q_OBJECT
public:
    explicit TimetableCard(QWidget *parent = nullptr);
    void setPeriod(const TimetablePeriod &period);
    void setEmptyLabel(const QString &label);

private:
    QLabel *mTitle = nullptr;
    QLabel *mSubtitle = nullptr;
};

class TimetablePage : public QWidget
{
    Q_OBJECT
public:
    explicit TimetablePage(QWidget *parent = nullptr);

    void setJsonManager(JsonManager *manager);
    void setSchoolPeriods(const SchoolPeriodsData &data);
    void setSettings(const SettingsData &settings);

private:
    void createLayout();
    void rebuildTimetable();
    QVector<TimetablePeriod> buildDay(const QString &weekKey, const QString &dayName) const;

    JsonManager *mJsonManager = nullptr;
    SchoolPeriodsData mData;
    SettingsData mSettings;

    QButtonGroup *mWeekGroup = nullptr;
    QGridLayout *mGridLayout = nullptr;
    QLabel *mWeekLabel = nullptr;
};
