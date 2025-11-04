#pragma once

#include "jsonmanager.h"
#include "models.h"

#include <QColor>
#include <QComboBox>
#include <QDateEdit>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QStackedWidget>
#include <QTextEdit>
#include <QTimeEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class ActivitiesWidget;
class DonutChartWidget;

class HomePage : public QWidget
{
    Q_OBJECT
public:
    explicit HomePage(QWidget *parent = nullptr);

    void setJsonManager(JsonManager *manager);
    void setActivities(const QVector<Activity> &activities);
    void setSchoolPeriods(const SchoolPeriodsData &data);
    void setSettings(const SettingsData &settings);

signals:
    void activitiesChanged(const QVector<Activity> &activities);

private:
    void createLayout();
    void refreshDonut();

    JsonManager *mJsonManager = nullptr;
    ActivitiesWidget *mActivitiesWidget = nullptr;
    DonutChartWidget *mDonutChart = nullptr;
    QVector<Activity> mActivities;
    SchoolPeriodsData mSchoolPeriods;
    SettingsData mSettings;
};

class ActivityRowWidget : public QFrame
{
    Q_OBJECT
public:
    explicit ActivityRowWidget(const Activity &activity, QWidget *parent = nullptr);

    void setActivity(const Activity &activity);
    Activity activity() const;

signals:
    void editRequested(const Activity &activity);
    void deleteRequested(const QString &activityId);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void toggleExpanded();

private:
    void buildUi();
    void updateUi();

    Activity mActivity;
    bool mExpanded = false;
    QLabel *mTitleLabel = nullptr;
    QLabel *mTimeLabel = nullptr;
    QLabel *mDescriptionLabel = nullptr;
    QPushButton *mEditButton = nullptr;
    QPushButton *mDeleteButton = nullptr;
    QWidget *mDetailContainer = nullptr;
    QPropertyAnimation *mDetailAnimation = nullptr;
};

class ActivitiesWidget : public QFrame
{
    Q_OBJECT
public:
    explicit ActivitiesWidget(QWidget *parent = nullptr);

    void setActivities(const QVector<Activity> &activities);

signals:
    void activityCreated(const Activity &activity);
    void editActivityRequested(const Activity &activity);
    void deleteActivityRequested(const QString &activityId);

private:
    void enterCreationMode();
    void exitCreationMode();
    void resetCreationForm();
    void rebuildUi();
    void updateColorButton();

    QVector<Activity> mActivities;
    QStackedWidget *mStack = nullptr;
    QWidget *mListPage = nullptr;
    QWidget *mCreationPage = nullptr;
    QVBoxLayout *mListLayout = nullptr;
    QPushButton *mNewButton = nullptr;
    QLineEdit *mTitleEdit = nullptr;
    QTextEdit *mDescriptionEdit = nullptr;
    QDateEdit *mDateEdit = nullptr;
    QTimeEdit *mStartTimeEdit = nullptr;
    QTimeEdit *mEndTimeEdit = nullptr;
    QPushButton *mColorButton = nullptr;
    QColor mSelectedColor = QColor("#4ECDC4");
};

class DonutChartWidget : public QWidget
{
    Q_OBJECT
public:
    enum class Mode
    {
        Activities,
        Timetable,
        Combined
    };

    explicit DonutChartWidget(QWidget *parent = nullptr);

    void setActivities(const QVector<Activity> &activities);
    void setPeriods(const QVector<TimetablePeriod> &periods);
    void setMode(Mode mode);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void updateClock();
    void rebuildArcs();
    void drawBaseDonut(QPainter &painter, const QRectF &outerRect, const QRectF &innerRect) const;
    void drawArcs(QPainter &painter, const QRectF &outerRect, const QRectF &innerRect);
    void drawHand(QPainter &painter, const QRectF &outerRect, const QRectF &innerRect) const;
    void drawLabels(QPainter &painter, const QRectF &outerRect) const;
    double minutesToAngle(double minutes) const;

    QVector<Activity> mActivities;
    QVector<TimetablePeriod> mPeriods;
    QVector<DonutArc> mActivityArcs;
    QVector<DonutArc> mPeriodArcs;
    Mode mMode = Mode::Activities;
    QTimer *mTimer = nullptr;
    QDateTime mNow;
};
