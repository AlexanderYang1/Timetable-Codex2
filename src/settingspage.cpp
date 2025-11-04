#include "settingspage.h"

#include <QBoxLayout>
#include <QButtonGroup>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    createLayout();
}

void SettingsPage::setJsonManager(JsonManager *manager)
{
    mJsonManager = manager;
}

void SettingsPage::setSettings(const SettingsData &settings)
{
    mSettings = settings;
    updateControls();
}

void SettingsPage::createLayout()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(24);

    auto *title = new QLabel(tr("Settings"), this);
    title->setStyleSheet(QStringLiteral("font-size: 24px; font-weight: bold;"));
    layout->addWidget(title);

    auto *weekSection = new QFrame(this);
    weekSection->setStyleSheet(QStringLiteral("QFrame { border: 1px solid #E0E0E0; border-radius: 16px; background: #FFFFFF; padding: 16px; }") );
    auto *weekLayout = new QVBoxLayout(weekSection);
    weekLayout->setSpacing(12);

    auto *weekLabel = new QLabel(tr("Timetable Week Settings"), weekSection);
    weekLabel->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: bold;"));
    weekLayout->addWidget(weekLabel);

    auto *weekControls = new QHBoxLayout();
    weekControls->setSpacing(12);
    auto *weekALabel = new QPushButton(tr("A Week"), weekSection);
    auto *weekBLabel = new QPushButton(tr("B Week"), weekSection);
    weekALabel->setCheckable(true);
    weekBLabel->setCheckable(true);
    weekALabel->setStyleSheet(QStringLiteral("QPushButton { padding: 8px 16px; border-radius: 18px; border: 1px solid #E0E0E0; }"
                                          "QPushButton:checked { background: #000000; color: #FFFFFF; }"));
    weekBLabel->setStyleSheet(weekALabel->styleSheet());

    mWeekGroup = new QButtonGroup(this);
    mWeekGroup->addButton(weekALabel, 0);
    mWeekGroup->addButton(weekBLabel, 1);

    weekControls->addWidget(new QLabel(tr("Current Week:"), weekSection));
    weekControls->addWidget(weekALabel);
    weekControls->addWidget(weekBLabel);
    weekControls->addStretch(1);

    weekLayout->addLayout(weekControls);

    layout->addWidget(weekSection);

    auto *yearSection = new QFrame(this);
    yearSection->setStyleSheet(weekSection->styleSheet());
    auto *yearLayout = new QVBoxLayout(yearSection);
    yearLayout->setSpacing(12);

    auto *yearLabel = new QLabel(tr("Wednesday Schedule"), yearSection);
    yearLabel->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: bold;"));
    yearLayout->addWidget(yearLabel);

    auto *yearControls = new QHBoxLayout();
    yearControls->setSpacing(12);
    auto *year10 = new QPushButton(tr("Year 10"), yearSection);
    auto *year11 = new QPushButton(tr("Year 11"), yearSection);
    year10->setCheckable(true);
    year11->setCheckable(true);
    year10->setStyleSheet(weekALabel->styleSheet());
    year11->setStyleSheet(weekALabel->styleSheet());

    mYearGroup = new QButtonGroup(this);
    mYearGroup->addButton(year10, 10);
    mYearGroup->addButton(year11, 11);

    yearControls->addWidget(new QLabel(tr("Year Level:"), yearSection));
    yearControls->addWidget(year10);
    yearControls->addWidget(year11);
    yearControls->addStretch(1);

    yearLayout->addLayout(yearControls);

    layout->addWidget(yearSection);
    layout->addStretch(1);

    connect(mWeekGroup, &QButtonGroup::idToggled, this, [this](int id, bool checked) {
        if (!checked)
        {
            return;
        }
        mSettings.currentWeek = id == 0 ? QStringLiteral("A") : QStringLiteral("B");
        emit settingsChanged(mSettings);
    });

    connect(mYearGroup, &QButtonGroup::idToggled, this, [this](int id, bool checked) {
        if (!checked)
        {
            return;
        }
        mSettings.yearLevel = id;
        emit settingsChanged(mSettings);
    });
}

void SettingsPage::updateControls()
{
    if (mWeekGroup)
    {
        const int id = mSettings.currentWeek.compare(QStringLiteral("B"), Qt::CaseInsensitive) == 0 ? 1 : 0;
        if (auto *button = mWeekGroup->button(id))
        {
            button->setChecked(true);
        }
    }

    if (mYearGroup)
    {
        const int id = mSettings.yearLevel >= 11 ? 11 : 10;
        if (auto *button = mYearGroup->button(id))
        {
            button->setChecked(true);
        }
    }
}
