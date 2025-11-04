#include "timetablepage.h"

#include <QBoxLayout>
#include <QButtonGroup>
#include <QDate>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QStyleOption>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>

namespace
{
const QStringList kDays = {QStringLiteral("Monday"), QStringLiteral("Tuesday"), QStringLiteral("Wednesday"), QStringLiteral("Thursday"), QStringLiteral("Friday")};
}

TimetableCard::TimetableCard(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("TimetableCard"));
    setFrameShape(QFrame::NoFrame);
    setStyleSheet(QStringLiteral("#TimetableCard { border-radius: 12px; background: #FFFFFF; border: 1px solid #E0E0E0; }") );

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    mTitle = new QLabel(this);
    mTitle->setWordWrap(true);
    mTitle->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: bold;"));

    mSubtitle = new QLabel(this);
    mSubtitle->setStyleSheet(QStringLiteral("color: #666666;"));
    mSubtitle->setWordWrap(true);

    layout->addWidget(mTitle);
    layout->addWidget(mSubtitle);
}

void TimetableCard::setPeriod(const TimetablePeriod &period)
{
    QString subtitle;
    if (!period.room.isEmpty())
    {
        subtitle = QStringLiteral("%1\n%2 - %3")
                       .arg(period.room, period.startTime.toString("HH:mm"), period.endTime.toString("HH:mm"));
    }
    else
    {
        subtitle = QStringLiteral("%1 - %2")
                       .arg(period.startTime.toString("HH:mm"), period.endTime.toString("HH:mm"));
    }

    mTitle->setText(period.subjectName.isEmpty() ? period.periodKey : period.subjectName);
    mSubtitle->setText(subtitle);

    QColor background = period.color.isValid() ? period.color : QColor("#F5F5F5");
    if (period.isSpecial)
    {
        background = background.lighter(140);
        QFont font = mTitle->font();
        font.setItalic(true);
        mTitle->setFont(font);
    }
    else
    {
        QFont font = mTitle->font();
        font.setItalic(false);
        mTitle->setFont(font);
    }

    setStyleSheet(QStringLiteral("#TimetableCard { border-radius: 12px; border: 1px solid #E0E0E0; background: %1; }")
                      .arg(background.name(QColor::HexRgb)));
}

void TimetableCard::setEmptyLabel(const QString &label)
{
    mTitle->setText(label);
    mSubtitle->setText(tr("No Period"));
    setStyleSheet(QStringLiteral("#TimetableCard { border-radius: 12px; border: 1px dashed #E0E0E0; background: #FAFAFA; }") );
}

TimetablePage::TimetablePage(QWidget *parent)
    : QWidget(parent)
{
    createLayout();
}

void TimetablePage::setJsonManager(JsonManager *manager)
{
    mJsonManager = manager;
}

void TimetablePage::setSchoolPeriods(const SchoolPeriodsData &data)
{
    mData = data;
    rebuildTimetable();
}

void TimetablePage::setSettings(const SettingsData &settings)
{
    mSettings = settings;
    rebuildTimetable();
}

void TimetablePage::createLayout()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    auto *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);

    auto *title = new QLabel(tr("Timetable"), this);
    title->setStyleSheet(QStringLiteral("font-size: 24px; font-weight: bold;"));

    headerLayout->addWidget(title);
    headerLayout->addStretch(1);

    mWeekLabel = new QLabel(this);
    headerLayout->addWidget(mWeekLabel);

    mWeekGroup = new QButtonGroup(this);
    auto *weekALabel = new QPushButton(tr("A Week"), this);
    weekALabel->setCheckable(true);
    auto *weekBLabel = new QPushButton(tr("B Week"), this);
    weekBLabel->setCheckable(true);
    weekALabel->setStyleSheet(QStringLiteral("QPushButton { padding: 6px 12px; border-radius: 16px; border: 1px solid #E0E0E0; }"
                                          "QPushButton:checked { background: #000000; color: #FFFFFF; }"));
    weekBLabel->setStyleSheet(weekALabel->styleSheet());

    mWeekGroup->addButton(weekALabel, 0);
    mWeekGroup->addButton(weekBLabel, 1);

    headerLayout->addWidget(weekALabel);
    headerLayout->addWidget(weekBLabel);

    mainLayout->addLayout(headerLayout);

    auto *gridWidget = new QWidget(this);
    mGridLayout = new QGridLayout(gridWidget);
    mGridLayout->setSpacing(16);
    mGridLayout->setContentsMargins(0, 0, 0, 0);

    mainLayout->addWidget(gridWidget);

    connect(mWeekGroup, &QButtonGroup::idToggled, this, [this](int id, bool checked) {
        if (!checked)
        {
            return;
        }
        mSettings.currentWeek = id == 0 ? QStringLiteral("A") : QStringLiteral("B");
        rebuildTimetable();
        if (mJsonManager)
        {
            mJsonManager->saveSettings(mSettings);
        }
    });
}

void TimetablePage::rebuildTimetable()
{
    if (!mGridLayout)
    {
        return;
    }

    const QString weekKey = mSettings.currentWeek.isEmpty() ? QStringLiteral("A") : mSettings.currentWeek;

    mWeekLabel->setText(tr("Current Week: %1").arg(weekKey));

    if (mWeekGroup)
    {
        const int id = weekKey.compare(QStringLiteral("B"), Qt::CaseInsensitive) == 0 ? 1 : 0;
        if (auto *button = mWeekGroup->button(id))
        {
            button->setChecked(true);
        }
    }

    while (mGridLayout->count() > 0)
    {
        QLayoutItem *item = mGridLayout->takeAt(0);
        if (item->widget())
        {
            item->widget()->deleteLater();
        }
        delete item;
    }

    int column = 0;
    for (const auto &day : kDays)
    {
        auto *dayLabel = new QLabel(day, this);
        dayLabel->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 16px;"));
        mGridLayout->addWidget(dayLabel, 0, column);

        const auto periods = buildDay(weekKey, day);
        int row = 1;
        if (periods.isEmpty())
        {
            auto *card = new TimetableCard(this);
            card->setEmptyLabel(tr("No Periods"));
            mGridLayout->addWidget(card, row, column);
        }
        else
        {
            for (const auto &period : periods)
            {
                auto *card = new TimetableCard(this);
                card->setPeriod(period);
                mGridLayout->addWidget(card, row, column);
                ++row;
            }
        }
        ++column;
    }
}

QVector<TimetablePeriod> TimetablePage::buildDay(const QString &weekKey, const QString &dayName) const
{
    QVector<TimetablePeriod> result;
    if (!mData.weeks.contains(weekKey))
    {
        return result;
    }

    const WeekSchedule &week = mData.weeks.value(weekKey);
    if (!week.days.contains(dayName))
    {
        return result;
    }

    const DaySchedule &day = week.days.value(dayName);
    QString templateName = day.templateName;
    if (templateName.contains(QStringLiteral("wednesday"), Qt::CaseInsensitive))
    {
        templateName = mSettings.yearLevel >= 11 ? QStringLiteral("wednesday_year11") : QStringLiteral("wednesday_year10");
    }

    if (!mData.templates.contains(templateName))
    {
        return result;
    }

    const TimetableTemplate &templ = mData.templates.value(templateName);
    QMap<QString, TimetableSubjectSlot> slotLookup;
    for (const auto &slot : day.slots)
    {
        slotLookup.insert(slot.periodKey, slot);
    }

    const QDate referenceDate = QDate::currentDate();
    for (const auto &periodTime : templ.periods)
    {
        TimetablePeriod period;
        period.periodKey = periodTime.label;
        period.startTime = QDateTime(referenceDate, periodTime.start);
        period.endTime = QDateTime(referenceDate, periodTime.end);

        if (slotLookup.contains(periodTime.label))
        {
            const auto slot = slotLookup.value(periodTime.label);
            period.subjectName = slot.subjectName;
            period.room = slot.room;
            if (mData.subjects.contains(slot.subjectName))
            {
                const auto subject = mData.subjects.value(slot.subjectName);
                period.teacher = subject.teacher;
                period.color = subject.color;
            }
        }
        else
        {
            period.isSpecial = true;
            period.subjectName = periodTime.label;
            period.room = {};
            period.teacher = {};
            period.color = QColor("#EFEFEF");
        }

        result.append(period);
    }

    return result;
}
