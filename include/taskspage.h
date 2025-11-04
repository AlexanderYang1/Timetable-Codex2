#pragma once

#include "models.h"

#include <QCheckBox>
#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QLineEdit>
#include <QMouseEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

class QShowEvent;

class TaskCardWidget : public QFrame
{
    Q_OBJECT
public:
    explicit TaskCardWidget(QWidget *parent = nullptr);
    void setTask(const Task &task, double progress);
    QString taskId() const;

signals:
    void clicked(const QString &taskId);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    Task mTask;
    QLabel *mTitle = nullptr;
    QLabel *mDescription = nullptr;
    QProgressBar *mProgress = nullptr;
};

class SubtaskRowWidget : public QFrame
{
    Q_OBJECT
public:
    explicit SubtaskRowWidget(QWidget *parent = nullptr);
    void setSubtask(const Subtask &subtask);
    Subtask subtask() const;

signals:
    void subtaskChanged(const Subtask &subtask);
    void deleteRequested(const QString &subtaskId);

private:
    void updateUi();

    Subtask mSubtask;
    QCheckBox *mCheck = nullptr;
    QLineEdit *mTitleEdit = nullptr;
    QTextEdit *mDescriptionEdit = nullptr;
    QDateTimeEdit *mDueEdit = nullptr;
    QDoubleSpinBox *mWeightSpin = nullptr;
    QPushButton *mDeleteButton = nullptr;
};

class TaskDetailView : public QWidget
{
    Q_OBJECT
public:
    explicit TaskDetailView(QWidget *parent = nullptr);

    void setTask(const Task &task);

signals:
    void taskUpdated(const Task &task);
    void taskDeleted(const QString &taskId);

private:
    void rebuildSubtasks();
    void recalculateProgress();

    Task mTask;
    QLineEdit *mTitleEdit = nullptr;
    QTextEdit *mDescriptionEdit = nullptr;
    QDateTimeEdit *mStartEdit = nullptr;
    QDateTimeEdit *mEndEdit = nullptr;
    QProgressBar *mProgressBar = nullptr;
    QPushButton *mDeleteButton = nullptr;
    QVBoxLayout *mSubtaskLayout = nullptr;
};

class TasksPage : public QWidget
{
    Q_OBJECT
public:
    explicit TasksPage(QWidget *parent = nullptr);

    void setJsonManager(JsonManager *manager);
    void setTasks(const QVector<Task> &tasks);
    void refreshFromHome(const QVector<Activity> &activities);

signals:
    void tasksChanged(const QVector<Task> &tasks);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void createLayout();
    void rebuildList();
    void openTaskDetail(const QString &taskId);
    void saveTask(const Task &task);
    void deleteTask(const QString &taskId);
    double taskProgress(const Task &task) const;

    JsonManager *mJsonManager = nullptr;
    QVector<Task> mTasks;

    QStackedWidget *mStack = nullptr;
    QWidget *mListPage = nullptr;
    TaskDetailView *mDetailPage = nullptr;
    QVBoxLayout *mListLayout = nullptr;
};
