#include "homepage.h"

#include "jsonmanager.h"

#include <QBoxLayout>
#include <QButtonGroup>
#include <QCalendarWidget>
#include <QColorDialog>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGraphicsDropShadowEffect>
#include <QGroupBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStyleOption>
#include <QTextEdit>
#include <QUuid>
#include <QAbstractAnimation>
#include <QEasingCurve>
#include <QPropertyAnimation>

#include <algorithm>
#include <QtMath>
#include <limits>

namespace
{
constexpr int kStandardPadding = 24;

bool validateActivityRange(const QDateTime &start, const QDateTime &end)
{
    return start.isValid() && end.isValid() && start < end && start >= QDateTime::currentDateTime();
}

class ActivityDialog : public QDialog
{
public:
    ActivityDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(tr("Activity"));
        setModal(true);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(16);

        auto *form = new QFormLayout();
        form->setLabelAlignment(Qt::AlignLeft);
        form->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);

        mTitleEdit = new QLineEdit(this);
        mDescriptionEdit = new QTextEdit(this);
        mDescriptionEdit->setFixedHeight(80);
        mStartEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
        mStartEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
        mEndEdit = new QDateTimeEdit(QDateTime::currentDateTime().addSecs(3600), this);
        mEndEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
        mColorButton = new QPushButton(tr("Choose Color"), this);

        form->addRow(tr("Title"), mTitleEdit);
        form->addRow(tr("Description"), mDescriptionEdit);
        form->addRow(tr("Start"), mStartEdit);
        form->addRow(tr("End"), mEndEdit);
        form->addRow(tr("Color"), mColorButton);

        layout->addLayout(form);

        auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
        layout->addWidget(buttonBox);

        connect(mColorButton, &QPushButton::clicked, this, [this]() {
            const QColor chosen = QColorDialog::getColor(mColor, this, tr("Activity Color"));
            if (chosen.isValid())
            {
                mColor = chosen;
                updateColorSwatch();
            }
        });

        connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
            if (!validateActivityRange(mStartEdit->dateTime(), mEndEdit->dateTime()))
            {
                QMessageBox::warning(this, tr("Invalid Range"), tr("Ensure the end time is after the start time and both are in the future."));
                return;
            }
            accept();
        });
        connect(buttonBox, &QDialogButtonBox::rejected, this, &ActivityDialog::reject);

        updateColorSwatch();
    }

    void setActivity(const Activity &activity)
    {
        mActivityId = activity.id;
        mTitleEdit->setText(activity.title);
        mDescriptionEdit->setPlainText(activity.description);
        mStartEdit->setDateTime(activity.startTime);
        mEndEdit->setDateTime(activity.endTime);
        mColor = activity.color;
        updateColorSwatch();
    }

    Activity activity() const
    {
        Activity activity;
        activity.id = mActivityId;
        activity.title = mTitleEdit->text();
        activity.description = mDescriptionEdit->toPlainText();
        activity.startTime = mStartEdit->dateTime();
        activity.endTime = mEndEdit->dateTime();
        activity.color = mColor;
        return activity;
    }

private:
    void updateColorSwatch()
    {
        if (!mColor.isValid())
        {
            mColor = QColor("#4ECDC4");
        }
        const QString style = QStringLiteral("QPushButton { background-color: %1; border-radius: 6px; padding: 8px; color: %2; }")
                                  .arg(mColor.name(QColor::HexRgb), mColor.lightness() < 140 ? QStringLiteral("white") : QStringLiteral("black"));
        mColorButton->setStyleSheet(style);
    }

    QString mActivityId;
    QLineEdit *mTitleEdit = nullptr;
    QTextEdit *mDescriptionEdit = nullptr;
    QDateTimeEdit *mStartEdit = nullptr;
    QDateTimeEdit *mEndEdit = nullptr;
    QPushButton *mColorButton = nullptr;
    QColor mColor = QColor("#4ECDC4");
};

QVector<Activity> sortActivities(const QVector<Activity> &activities)
{
    QVector<Activity> sorted = activities;
    std::sort(sorted.begin(), sorted.end(), [](const Activity &a, const Activity &b) {
        return a.startTime < b.startTime;
    });
    return sorted;
}
}

HomePage::HomePage(QWidget *parent)
    : QWidget(parent)
{
    createLayout();
}

void HomePage::setJsonManager(JsonManager *manager)
{
    mJsonManager = manager;
}

void HomePage::setActivities(const QVector<Activity> &activities)
{
    mActivities = sortActivities(activities);
    mActivitiesWidget->setActivities(mActivities);
    refreshDonut();
}

void HomePage::setSchoolPeriods(const SchoolPeriodsData &data)
{
    mSchoolPeriods = data;
    refreshDonut();
}

void HomePage::setSettings(const SettingsData &settings)
{
    mSettings = settings;
    refreshDonut();
}

void HomePage::createLayout()
{
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(kStandardPadding, kStandardPadding, kStandardPadding, kStandardPadding);
    mainLayout->setSpacing(24);

    auto *leftColumn = new QVBoxLayout();
    leftColumn->setSpacing(16);

    mActivitiesWidget = new ActivitiesWidget(this);
    leftColumn->addWidget(mActivitiesWidget);
    leftColumn->addStretch(1);

    auto *donutContainer = new QFrame(this);
    donutContainer->setObjectName(QStringLiteral("DonutContainer"));
    donutContainer->setStyleSheet(QStringLiteral("#DonutContainer { background: #FFFFFF; border: 1px solid #E0E0E0; border-radius: 16px; }") );

    auto *donutLayout = new QVBoxLayout(donutContainer);
    donutLayout->setContentsMargins(24, 24, 24, 24);
    donutLayout->setSpacing(16);

    mDonutChart = new DonutChartWidget(donutContainer);
    donutLayout->addWidget(mDonutChart, 1);

    auto *modeGroup = new QButtonGroup(this);
    modeGroup->setExclusive(true);

    auto *modeLayout = new QHBoxLayout();
    modeLayout->setSpacing(8);
    modeLayout->addStretch(1);

    const struct
    {
        QString label;
        DonutChartWidget::Mode mode;
    } modes[] = {{tr("Activities"), DonutChartWidget::Mode::Activities},
                 {tr("Timetable"), DonutChartWidget::Mode::Timetable},
                 {tr("Both"), DonutChartWidget::Mode::Combined}};

    int index = 0;
    for (const auto &mode : modes)
    {
        auto *button = new QPushButton(mode.label, donutContainer);
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        button->setStyleSheet(QStringLiteral("QPushButton { padding: 8px 16px; border-radius: 16px; border: 1px solid #E0E0E0; background: #FFFFFF; }"
                                            "QPushButton:checked { background: #000000; color: #FFFFFF; }"));
        modeGroup->addButton(button, index);
        modeLayout->addWidget(button);
        if (index == 0)
        {
            button->setChecked(true);
        }
        ++index;
    }

    modeLayout->addStretch(1);
    donutLayout->addLayout(modeLayout);

    mainLayout->addLayout(leftColumn, 2);
    mainLayout->addWidget(donutContainer, 3);

    connect(modeGroup, &QButtonGroup::idToggled, this, [this, modes](int id, bool checked) {
        if (!checked)
        {
            return;
        }
        mDonutChart->setMode(modes[id].mode);
    });

    connect(mActivitiesWidget, &ActivitiesWidget::activityCreated, this, [this](const Activity &activity) {
        Activity withId = activity;
        if (withId.id.isEmpty())
        {
            withId.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        }
        mActivities.append(withId);
        setActivities(mActivities);
        emit activitiesChanged(mActivities);
    });

    connect(mActivitiesWidget, &ActivitiesWidget::editActivityRequested, this, [this](const Activity &activity) {
        ActivityDialog dialog(this);
        dialog.setActivity(activity);
        if (dialog.exec() == QDialog::Accepted)
        {
            Activity updated = dialog.activity();
            for (auto &item : mActivities)
            {
                if (item.id == updated.id)
                {
                    item = updated;
                    break;
                }
            }
            setActivities(mActivities);
            emit activitiesChanged(mActivities);
        }
    });

    connect(mActivitiesWidget, &ActivitiesWidget::deleteActivityRequested, this, [this](const QString &activityId) {
        if (QMessageBox::question(this, tr("Delete Activity"), tr("Are you sure you want to delete this activity?")) == QMessageBox::Yes)
        {
            mActivities.erase(std::remove_if(mActivities.begin(), mActivities.end(), [&](const Activity &activity) {
                                  return activity.id == activityId;
                              }),
                              mActivities.end());
            setActivities(mActivities);
            emit activitiesChanged(mActivities);
        }
    });
}

void HomePage::refreshDonut()
{
    if (!mDonutChart)
    {
        return;
    }

    mDonutChart->setActivities(mActivities);

    if (mJsonManager)
    {
        const QDateTime from = QDateTime::currentDateTime();
        const QDateTime to = from.addSecs(12 * 3600);
        const auto periods = mJsonManager->upcomingPeriods(mSettings, from, to);
        mDonutChart->setPeriods(periods);
    }
}

ActivityRowWidget::ActivityRowWidget(const Activity &activity, QWidget *parent)
    : QFrame(parent)
    , mActivity(activity)
{
    setObjectName(QStringLiteral("ActivityRow"));
    setFrameShape(QFrame::NoFrame);
    setCursor(Qt::PointingHandCursor);
    setStyleSheet(QStringLiteral("#ActivityRow { background: #FFFFFF; border: 1px solid #E0E0E0; border-radius: 12px; }") );

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(12.0);
    shadow->setOffset(0.0, 3.0);
    shadow->setColor(QColor(0, 0, 0, 30));
    setGraphicsEffect(shadow);

    buildUi();

    if (mDetailContainer)
    {
        mDetailContainer->setVisible(false);
        mDetailContainer->setMaximumHeight(0);
        mDetailContainer->setMinimumHeight(0);
        mDetailAnimation = new QPropertyAnimation(mDetailContainer, "maximumHeight", this);
        mDetailAnimation->setDuration(220);
        mDetailAnimation->setEasingCurve(QEasingCurve::InOutQuad);
        connect(mDetailAnimation, &QPropertyAnimation::finished, this, [this]() {
            if (!mDetailContainer)
            {
                return;
            }
            if (mExpanded)
            {
                mDetailContainer->setMaximumHeight(std::numeric_limits<int>::max());
            }
            else
            {
                mDetailContainer->setVisible(false);
                mDetailContainer->setMaximumHeight(0);
            }
        });
    }

    updateUi();
}

void ActivityRowWidget::setActivity(const Activity &activity)
{
    mActivity = activity;
    updateUi();
}

Activity ActivityRowWidget::activity() const
{
    return mActivity;
}

void ActivityRowWidget::toggleExpanded()
{
    mExpanded = !mExpanded;
    if (!mDetailContainer || !mDetailAnimation)
    {
        updateUi();
        return;
    }

    if (mDetailAnimation->state() == QAbstractAnimation::Running)
    {
        mDetailAnimation->stop();
    }

    if (mExpanded)
    {
        mDetailContainer->setVisible(true);
        mDetailContainer->setMaximumHeight(0);
        mDetailAnimation->setStartValue(0);
        mDetailAnimation->setEndValue(mDetailContainer->sizeHint().height());
    }
    else
    {
        mDetailAnimation->setStartValue(mDetailContainer->height());
        mDetailAnimation->setEndValue(0);
    }

    mDetailAnimation->start();
    updateUi();
}

void ActivityRowWidget::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    auto *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(8);

    mTitleLabel = new QLabel(this);
    mTitleLabel->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 16px;"));

    mTimeLabel = new QLabel(this);
    mTimeLabel->setStyleSheet(QStringLiteral("color: #666666;"));

    headerLayout->addWidget(mTitleLabel);
    headerLayout->addStretch(1);
    headerLayout->addWidget(mTimeLabel);

    layout->addLayout(headerLayout);

    mDetailContainer = new QWidget(this);
    mDetailContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto *detailLayout = new QVBoxLayout(mDetailContainer);
    detailLayout->setContentsMargins(0, 0, 0, 0);
    detailLayout->setSpacing(8);

    mDescriptionLabel = new QLabel(mDetailContainer);
    mDescriptionLabel->setWordWrap(true);
    mDescriptionLabel->setStyleSheet(QStringLiteral("color: #888888;"));
    detailLayout->addWidget(mDescriptionLabel);

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(1);
    mEditButton = new QPushButton(tr("Edit"), mDetailContainer);
    mEditButton->setCursor(Qt::PointingHandCursor);
    mDeleteButton = new QPushButton(tr("Delete"), mDetailContainer);
    mDeleteButton->setCursor(Qt::PointingHandCursor);
    buttonLayout->addWidget(mEditButton);
    buttonLayout->addWidget(mDeleteButton);
    detailLayout->addLayout(buttonLayout);

    layout->addWidget(mDetailContainer);

    connect(mEditButton, &QPushButton::clicked, this, [this]() {
        emit editRequested(mActivity);
    });
    connect(mDeleteButton, &QPushButton::clicked, this, [this]() {
        emit deleteRequested(mActivity.id);
    });
}

void ActivityRowWidget::updateUi()
{
    mTitleLabel->setText(mActivity.title);
    mTimeLabel->setText(QStringLiteral("%1 - %2")
                            .arg(mActivity.startTime.toString("HH:mm"), mActivity.endTime.toString("HH:mm")));
    mDescriptionLabel->setText(mActivity.description);
}

void ActivityRowWidget::mousePressEvent(QMouseEvent *event)
{
    toggleExpanded();
    QFrame::mousePressEvent(event);
}

void ActivitiesWidget::rebuildUi()
{
    if (!mListLayout)
    {
        return;
    }

    QLayoutItem *item = nullptr;
    while ((item = mListLayout->takeAt(0)) != nullptr)
    {
        if (item->widget())
        {
            item->widget()->deleteLater();
        }
        delete item;
    }

    for (const auto &activity : mActivities)
    {
        auto *row = new ActivityRowWidget(activity, this);
        connect(row, &ActivityRowWidget::editRequested, this, &ActivitiesWidget::editActivityRequested);
        connect(row, &ActivityRowWidget::deleteRequested, this, &ActivitiesWidget::deleteActivityRequested);
        mListLayout->addWidget(row);
    }

    if (mActivities.isEmpty())
    {
        auto *placeholder = new QLabel(tr("No upcoming activities. Click + New Activity to add one."), this);
        placeholder->setStyleSheet(QStringLiteral("color: #888888;"));
        placeholder->setAlignment(Qt::AlignCenter);
        placeholder->setWordWrap(true);
        mListLayout->addWidget(placeholder);
    }

    mListLayout->addStretch(1);
}

ActivitiesWidget::ActivitiesWidget(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("ActivitiesWidget"));
    setStyleSheet(QStringLiteral("#ActivitiesWidget { background: #FFFFFF; border: 1px solid #E0E0E0; border-radius: 16px; }"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(8);

    auto *title = new QLabel(tr("Activities"), this);
    title->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: bold;"));

    mNewButton = new QPushButton(tr("+ New Activity"), this);
    mNewButton->setCursor(Qt::PointingHandCursor);
    mNewButton->setStyleSheet(QStringLiteral("QPushButton { background: #000000; color: #FFFFFF; padding: 8px 16px; border-radius: 18px; }"));

    headerLayout->addWidget(title);
    headerLayout->addStretch(1);
    headerLayout->addWidget(mNewButton);

    layout->addLayout(headerLayout);

    mStack = new QStackedWidget(this);
    layout->addWidget(mStack, 1);

    mListPage = new QWidget(this);
    auto *listPageLayout = new QVBoxLayout(mListPage);
    listPageLayout->setContentsMargins(0, 0, 0, 0);
    listPageLayout->setSpacing(12);
    mListLayout = new QVBoxLayout();
    mListLayout->setSpacing(12);
    listPageLayout->addLayout(mListLayout);
    listPageLayout->addStretch(1);
    mStack->addWidget(mListPage);

    mCreationPage = new QWidget(this);
    auto *creationLayout = new QVBoxLayout(mCreationPage);
    creationLayout->setContentsMargins(0, 0, 0, 0);
    creationLayout->setSpacing(16);

    auto *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignLeft);
    form->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);

    mTitleEdit = new QLineEdit(mCreationPage);
    mDescriptionEdit = new QTextEdit(mCreationPage);
    mDescriptionEdit->setFixedHeight(100);
    mDateEdit = new QDateEdit(QDate::currentDate(), mCreationPage);
    mDateEdit->setCalendarPopup(true);
    mDateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    mStartTimeEdit = new QTimeEdit(QTime::currentTime(), mCreationPage);
    mStartTimeEdit->setDisplayFormat(QStringLiteral("HH:mm"));
    mEndTimeEdit = new QTimeEdit(QTime::currentTime().addSecs(3600), mCreationPage);
    mEndTimeEdit->setDisplayFormat(QStringLiteral("HH:mm"));
    mColorButton = new QPushButton(tr("Choose Color"), mCreationPage);
    mColorButton->setCursor(Qt::PointingHandCursor);

    form->addRow(tr("Title"), mTitleEdit);
    form->addRow(tr("Description"), mDescriptionEdit);
    form->addRow(tr("Date"), mDateEdit);
    form->addRow(tr("Start Time"), mStartTimeEdit);
    form->addRow(tr("End Time"), mEndTimeEdit);
    form->addRow(tr("Color"), mColorButton);

    creationLayout->addLayout(form);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch(1);
    auto *saveButton = new QPushButton(tr("Save"), mCreationPage);
    saveButton->setCursor(Qt::PointingHandCursor);
    saveButton->setStyleSheet(QStringLiteral("QPushButton { background: #000000; color: #FFFFFF; padding: 8px 20px; border-radius: 18px; }"));
    auto *cancelButton = new QPushButton(tr("Cancel"), mCreationPage);
    cancelButton->setCursor(Qt::PointingHandCursor);
    cancelButton->setStyleSheet(QStringLiteral("QPushButton { background: #FFFFFF; border: 1px solid #E0E0E0; padding: 8px 20px; border-radius: 18px; }"));
    buttonRow->addWidget(cancelButton);
    buttonRow->addWidget(saveButton);
    creationLayout->addLayout(buttonRow);
    creationLayout->addStretch(1);

    mStack->addWidget(mCreationPage);

    updateColorButton();
    resetCreationForm();
    mStack->setCurrentWidget(mListPage);

    connect(mNewButton, &QPushButton::clicked, this, [this]() {
        enterCreationMode();
    });

    connect(mColorButton, &QPushButton::clicked, this, [this]() {
        const QColor chosen = QColorDialog::getColor(mSelectedColor, this, tr("Activity Color"));
        if (chosen.isValid())
        {
            mSelectedColor = chosen;
            updateColorButton();
        }
    });

    connect(cancelButton, &QPushButton::clicked, this, [this]() {
        exitCreationMode();
    });

    connect(saveButton, &QPushButton::clicked, this, [this]() {
        const QString title = mTitleEdit->text().trimmed();
        if (title.isEmpty())
        {
            QMessageBox::warning(this, tr("Missing Title"), tr("Please provide a title for the activity."));
            return;
        }

        const QDate date = mDateEdit->date();
        const QDateTime start(date, mStartTimeEdit->time());
        const QDateTime end(date, mEndTimeEdit->time());
        if (!validateActivityRange(start, end))
        {
            QMessageBox::warning(this,
                                 tr("Invalid Time Range"),
                                 tr("Ensure the start is in the future and the end time is after the start time."));
            return;
        }

        Activity activity;
        activity.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        activity.title = title;
        activity.description = mDescriptionEdit->toPlainText().trimmed();
        activity.startTime = start;
        activity.endTime = end;
        activity.color = mSelectedColor;

        emit activityCreated(activity);
        exitCreationMode();
    });
}

void ActivitiesWidget::setActivities(const QVector<Activity> &activities)
{
    mActivities = activities;
    rebuildUi();
}

void ActivitiesWidget::enterCreationMode()
{
    resetCreationForm();
    if (mStack && mCreationPage)
    {
        mStack->setCurrentWidget(mCreationPage);
    }
    if (mNewButton)
    {
        mNewButton->setVisible(false);
    }
}

void ActivitiesWidget::exitCreationMode()
{
    if (mStack && mListPage)
    {
        mStack->setCurrentWidget(mListPage);
    }
    if (mNewButton)
    {
        mNewButton->setVisible(true);
    }
    resetCreationForm();
}

void ActivitiesWidget::resetCreationForm()
{
    if (!mTitleEdit)
    {
        return;
    }

    mTitleEdit->clear();
    mDescriptionEdit->clear();

    const QDateTime now = QDateTime::currentDateTime();
    const QDateTime start = now.addSecs(15 * 60);
    const QDateTime end = start.addSecs(60 * 60);
    mDateEdit->setDate(start.date());
    mStartTimeEdit->setTime(start.time());
    mEndTimeEdit->setTime(end.time());
    mSelectedColor = QColor("#4ECDC4");
    updateColorButton();
}

void ActivitiesWidget::updateColorButton()
{
    if (!mColorButton)
    {
        return;
    }

    const QString style = QStringLiteral(
        "QPushButton { padding: 8px 16px; border-radius: 12px; border: 1px solid #E0E0E0; background: %1; color: %2; }")
                            .arg(mSelectedColor.name(QColor::HexRgb),
                                 mSelectedColor.lightness() < 140 ? QStringLiteral("#FFFFFF") : QStringLiteral("#000000"));
    mColorButton->setStyleSheet(style);
}

DonutChartWidget::DonutChartWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(360, 360);
    mTimer = new QTimer(this);
    mTimer->setInterval(1000);
    connect(mTimer, &QTimer::timeout, this, &DonutChartWidget::updateClock);
    mTimer->start();
    updateClock();
}

void DonutChartWidget::setActivities(const QVector<Activity> &activities)
{
    mActivities = activities;
    rebuildArcs();
    update();
}

void DonutChartWidget::setPeriods(const QVector<TimetablePeriod> &periods)
{
    mPeriods = periods;
    rebuildArcs();
    update();
}

void DonutChartWidget::setMode(Mode mode)
{
    if (mMode == mode)
    {
        return;
    }
    mMode = mode;
    update();
}

void DonutChartWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const double side = qMin(width(), height()) * 0.8;
    const QPointF center = rect().center();
    const QRectF outerRect(center.x() - side / 2.0, center.y() - side / 2.0, side, side);
    const double thickness = side * 0.20;
    const QRectF innerRect = outerRect.adjusted(thickness, thickness, -thickness, -thickness);

    drawBaseDonut(painter, outerRect, innerRect);
    drawArcs(painter, outerRect, innerRect);
    drawHand(painter, outerRect, innerRect);
    drawLabels(painter, outerRect);
}

void DonutChartWidget::updateClock()
{
    mNow = QDateTime::currentDateTime();
    rebuildArcs();
    update();
}

void DonutChartWidget::rebuildArcs()
{
    if (!mNow.isValid())
    {
        mNow = QDateTime::currentDateTime();
    }
    const QDateTime windowEnd = mNow.addSecs(12 * 3600);

    mActivityArcs.clear();
    for (const auto &activity : mActivities)
    {
        const auto start = std::max(activity.startTime, mNow);
        const auto end = std::min(activity.endTime, windowEnd);
        if (end <= start)
        {
            continue;
        }
        DonutArc arc;
        arc.startTime = start;
        arc.endTime = end;
        arc.color = activity.color;
        arc.label = activity.title;
        arc.category = QStringLiteral("Activity");
        mActivityArcs.append(arc);
    }

    mPeriodArcs.clear();
    for (const auto &period : mPeriods)
    {
        const auto start = std::max(period.startTime, mNow);
        const auto end = std::min(period.endTime, windowEnd);
        if (end <= start)
        {
            continue;
        }
        DonutArc arc;
        arc.startTime = start;
        arc.endTime = end;
        arc.color = period.color.isValid() ? period.color : QColor("#E0E0E0");
        arc.label = period.subjectName;
        arc.category = QStringLiteral("Timetable");
        mPeriodArcs.append(arc);
    }
}

void DonutChartWidget::drawBaseDonut(QPainter &painter, const QRectF &outerRect, const QRectF &innerRect) const
{
    QPen pen(QColor("#DDDDDD"));
    pen.setWidthF((outerRect.width() - innerRect.width()) / 2.0);
    pen.setCapStyle(Qt::FlatCap);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawArc(outerRect, 0, 360 * 16);
}

void DonutChartWidget::drawArcs(QPainter &painter, const QRectF &outerRect, const QRectF &innerRect)
{
    const double bandThickness = (outerRect.width() - innerRect.width()) / 2.0;

    auto drawArcList = [&](const QVector<DonutArc> &arcs, double widthFactor, double inset) {
        QPen pen;
        pen.setCapStyle(Qt::FlatCap);
        pen.setWidthF(bandThickness * widthFactor);
        for (const auto &arc : arcs)
        {
            pen.setColor(arc.color);
            painter.setPen(pen);
            const QRectF arcRect = outerRect.adjusted(inset, inset, -inset, -inset);
            const double startMinutes = mNow.secsTo(arc.startTime) / 60.0;
            const double spanMinutes = arc.startTime.secsTo(arc.endTime) / 60.0;
            const double startAngle = minutesToAngle(startMinutes);
            const double spanAngle = spanMinutes / 720.0 * 360.0;
            painter.drawArc(arcRect, static_cast<int>(startAngle * 16), static_cast<int>(-spanAngle * 16));
        }
    };

    if (mMode == Mode::Activities)
    {
        drawArcList(mActivityArcs, 0.85, bandThickness * 0.3);
    }
    else if (mMode == Mode::Timetable)
    {
        drawArcList(mPeriodArcs, 0.85, bandThickness * 0.3);
    }
    else
    {
        drawArcList(mPeriodArcs, 0.5, bandThickness * 0.75);
        drawArcList(mActivityArcs, 0.5, bandThickness * 0.25);
    }
}

void DonutChartWidget::drawHand(QPainter &painter, const QRectF &outerRect, const QRectF &innerRect) const
{
    painter.save();

    const QPointF center = outerRect.center();
    const double outerRadius = outerRect.width() / 2.0;
    const double innerRadius = innerRect.width() / 2.0;

    const double currentAngle = minutesToAngle(0);
    const double radians = qDegreesToRadians(currentAngle);

    const QPointF tip(center.x() + outerRadius * std::cos(radians), center.y() + outerRadius * std::sin(radians));

    QPen pen(QColor("#000000"));
    pen.setWidthF(4.0);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.drawLine(center, tip);

    QConicalGradient gradient(center, -currentAngle);
    gradient.setColorAt(0.0, QColor(0, 0, 0, 180));
    gradient.setColorAt(10.0 / 360.0, QColor(0, 0, 0, 0));

    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawPie(outerRect, static_cast<int>((currentAngle - 10.0) * 16), static_cast<int>(10.0 * 16));

    painter.setBrush(QColor("#FFFFFF"));
    painter.drawEllipse(innerRect);

    const double labelRadius = innerRadius * 0.6;
    const double oppositeRadians = qDegreesToRadians(currentAngle + 180.0);
    const QPointF labelCenter(center.x() + labelRadius * std::cos(oppositeRadians),
                              center.y() + labelRadius * std::sin(oppositeRadians));
    QRectF labelRect(labelCenter.x() - 52.0, labelCenter.y() - 18.0, 104.0, 36.0);

    painter.setBrush(QColor(255, 255, 255, 230));
    painter.drawRoundedRect(labelRect, 14.0, 14.0);

    painter.setPen(QColor("#000000"));
    painter.setFont(QFont(QStringLiteral("Helvetica"), 16, QFont::Bold));
    painter.drawText(labelRect, Qt::AlignCenter, mNow.toString("HH:mm:ss"));

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#000000"));
    painter.drawEllipse(QRectF(center.x() - 4.0, center.y() - 4.0, 8.0, 8.0));

    painter.restore();
}

void DonutChartWidget::drawLabels(QPainter &painter, const QRectF &outerRect) const
{
    painter.setPen(QColor("#000000"));
    painter.setFont(QFont(QStringLiteral("Helvetica"), 10, QFont::Medium));

    const QPointF center = outerRect.center();
    const double radius = outerRect.width() / 2.0 + 24;

    for (int i = 0; i <= 12; ++i)
    {
        const double minutes = i * 60.0;
        const double angle = minutesToAngle(minutes);
        const double radians = qDegreesToRadians(angle);
        const QPointF pos(center.x() + radius * std::cos(radians), center.y() + radius * std::sin(radians));
        const QDateTime labelTime = mNow.addSecs(static_cast<qint64>(minutes * 60));
        painter.drawText(QRectF(pos.x() - 20, pos.y() - 10, 40, 20), Qt::AlignCenter, labelTime.toString("HH"));
    }
}

double DonutChartWidget::minutesToAngle(double minutes) const
{
    return -90.0 + (minutes / 720.0) * 360.0;
}
