#include "taskspage.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QShowEvent>
#include <QSizePolicy>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QSignalBlocker>
#include <QUuid>

#include <algorithm>
#include <cmath>
#include <QtGlobal>

namespace
{
double computeProgress(const Task &task)
{
    double completedWeight = 0.0;
    double totalWeight = 0.0;
    for (const auto &subtask : task.subtasks)
    {
        totalWeight += std::max(0.0, subtask.weighting);
        if (subtask.completed)
        {
            completedWeight += std::max(0.0, subtask.weighting);
        }
    }
    if (totalWeight <= 0.0)
    {
        return 0.0;
    }
    return (completedWeight / totalWeight) * 100.0;
}

class TaskDialog : public QDialog
{
public:
    TaskDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(tr("Task"));
        setModal(true);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(16);

        auto *form = new QFormLayout();
        form->setLabelAlignment(Qt::AlignLeft);

        mTitleEdit = new QLineEdit(this);
        mDescriptionEdit = new QTextEdit(this);
        mDescriptionEdit->setFixedHeight(100);
        mStartEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
        mStartEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
        mEndEdit = new QDateTimeEdit(QDateTime::currentDateTime().addSecs(7200), this);
        mEndEdit->setDisplayFormat("yyyy-MM-dd HH:mm");

        form->addRow(tr("Title"), mTitleEdit);
        form->addRow(tr("Description"), mDescriptionEdit);
        form->addRow(tr("Start"), mStartEdit);
        form->addRow(tr("End"), mEndEdit);

        layout->addLayout(form);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
        layout->addWidget(buttons);

        connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
            if (mTitleEdit->text().trimmed().isEmpty())
            {
                QMessageBox::warning(this, tr("Missing Title"), tr("Please enter a title for the task."));
                return;
            }
            if (mStartEdit->dateTime() >= mEndEdit->dateTime())
            {
                QMessageBox::warning(this, tr("Invalid Range"), tr("End time must be after the start time."));
                return;
            }
            if (mStartEdit->dateTime() < QDateTime::currentDateTime())
            {
                QMessageBox::warning(this, tr("Invalid Start"), tr("Start time cannot be in the past."));
                return;
            }
            accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, this, &TaskDialog::reject);
    }

    void setTask(const Task &task)
    {
        mTaskId = task.id;
        mTitleEdit->setText(task.title);
        mDescriptionEdit->setPlainText(task.description);
        mStartEdit->setDateTime(task.startTime);
        mEndEdit->setDateTime(task.endTime);
    }

    Task task() const
    {
        Task task;
        task.id = mTaskId;
        task.title = mTitleEdit->text().trimmed();
        task.description = mDescriptionEdit->toPlainText();
        task.startTime = mStartEdit->dateTime();
        task.endTime = mEndEdit->dateTime();
        return task;
    }

private:
    QString mTaskId;
    QLineEdit *mTitleEdit = nullptr;
    QTextEdit *mDescriptionEdit = nullptr;
    QDateTimeEdit *mStartEdit = nullptr;
    QDateTimeEdit *mEndEdit = nullptr;
};
}

TaskCardWidget::TaskCardWidget(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("TaskCardWidget"));
    setStyleSheet(QStringLiteral("#TaskCardWidget { background: #FFFFFF; border-radius: 16px; border: 1px solid #E0E0E0; }") );
    setCursor(Qt::PointingHandCursor);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    mTitle = new QLabel(this);
    mTitle->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: bold;"));

    mDescription = new QLabel(this);
    mDescription->setWordWrap(true);
    mDescription->setStyleSheet(QStringLiteral("color: #666666;"));

    mProgress = new QProgressBar(this);
    mProgress->setRange(0, 100);
    mProgress->setTextVisible(true);
    mProgress->setStyleSheet(QStringLiteral("QProgressBar { background: #EEEEEE; border-radius: 10px; padding: 3px; }"
                                             "QProgressBar::chunk { background: #000000; border-radius: 10px; }") );

    layout->addWidget(mTitle);
    layout->addWidget(mDescription);
    layout->addWidget(mProgress);
}

void TaskCardWidget::setTask(const Task &task, double progress)
{
    mTask = task;
    mTitle->setText(task.title);
    mDescription->setText(task.description.isEmpty() ? tr("No description provided.") : task.description);
    mProgress->setValue(static_cast<int>(std::round(progress)));
    mProgress->setFormat(tr("%1% Completed").arg(QString::number(progress, 'f', 0)));
}

QString TaskCardWidget::taskId() const
{
    return mTask.id;
}

void TaskCardWidget::mousePressEvent(QMouseEvent *event)
{
    emit clicked(mTask.id);
    QFrame::mousePressEvent(event);
}

SubtaskRowWidget::SubtaskRowWidget(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("SubtaskRowWidget"));
    setStyleSheet(QStringLiteral("#SubtaskRowWidget { border: 1px solid #E0E0E0; border-radius: 12px; background: #FFFFFF; }") );

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    mCheck = new QCheckBox(this);
    layout->addWidget(mCheck);

    auto *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(6);

    mTitleEdit = new QLineEdit(this);
    mTitleEdit->setPlaceholderText(tr("Subtask title"));
    infoLayout->addWidget(mTitleEdit);

    mDescriptionEdit = new QTextEdit(this);
    mDescriptionEdit->setPlaceholderText(tr("Description"));
    mDescriptionEdit->setFixedHeight(60);
    infoLayout->addWidget(mDescriptionEdit);

    layout->addLayout(infoLayout, 1);

    mDueEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    mDueEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    layout->addWidget(mDueEdit);

    mWeightSpin = new QDoubleSpinBox(this);
    mWeightSpin->setRange(0.0, 1000.0);
    mWeightSpin->setDecimals(2);
    mWeightSpin->setValue(1.0);
    layout->addWidget(mWeightSpin);

    mDeleteButton = new QPushButton(tr("Delete"), this);
    mDeleteButton->setCursor(Qt::PointingHandCursor);
    layout->addWidget(mDeleteButton);

    connect(mCheck, &QCheckBox::toggled, this, [this]() {
        mSubtask.completed = mCheck->isChecked();
        emit subtaskChanged(mSubtask);
    });
    connect(mTitleEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        mSubtask.title = text;
        emit subtaskChanged(mSubtask);
    });
    connect(mDescriptionEdit, &QTextEdit::textChanged, this, [this]() {
        mSubtask.description = mDescriptionEdit->toPlainText();
        emit subtaskChanged(mSubtask);
    });
    connect(mDueEdit, &QDateTimeEdit::dateTimeChanged, this, [this](const QDateTime &value) {
        mSubtask.dueTime = value;
        emit subtaskChanged(mSubtask);
    });
    connect(mWeightSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        mSubtask.weighting = value;
        emit subtaskChanged(mSubtask);
    });
    connect(mDeleteButton, &QPushButton::clicked, this, [this]() {
        if (QMessageBox::question(this, tr("Delete Subtask"), tr("Are you sure you want to delete this subtask?")) == QMessageBox::Yes)
        {
            emit deleteRequested(mSubtask.id);
        }
    });
}

void SubtaskRowWidget::setSubtask(const Subtask &subtask)
{
    mSubtask = subtask;
    updateUi();
}

Subtask SubtaskRowWidget::subtask() const
{
    return mSubtask;
}

void SubtaskRowWidget::updateUi()
{
    mCheck->setChecked(mSubtask.completed);
    mTitleEdit->setText(mSubtask.title);
    mDescriptionEdit->setText(mSubtask.description);
    mDueEdit->setDateTime(mSubtask.dueTime);
    mWeightSpin->setValue(mSubtask.weighting);
}

TaskDetailView::TaskDetailView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(24);

    auto *leftColumn = new QVBoxLayout();
    leftColumn->setSpacing(12);

    mTitleEdit = new QLineEdit(this);
    mTitleEdit->setStyleSheet(QStringLiteral("font-size: 24px; font-weight: bold;"));
    mDescriptionEdit = new QTextEdit(this);
    mDescriptionEdit->setFixedHeight(120);
    mStartEdit = new QDateTimeEdit(this);
    mStartEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    mEndEdit = new QDateTimeEdit(this);
    mEndEdit->setDisplayFormat("yyyy-MM-dd HH:mm");

    mProgressBar = new QProgressBar(this);
    mProgressBar->setRange(0, 100);
    mProgressBar->setTextVisible(true);

    mDeleteButton = new QPushButton(tr("Delete Task"), this);
    mDeleteButton->setStyleSheet(QStringLiteral("QPushButton { background: #E53935; color: white; padding: 10px 16px; border-radius: 16px; }") );
    mDeleteButton->setCursor(Qt::PointingHandCursor);

    leftColumn->addWidget(mTitleEdit);
    leftColumn->addWidget(mDescriptionEdit);
    leftColumn->addWidget(mStartEdit);
    leftColumn->addWidget(mEndEdit);
    leftColumn->addWidget(mProgressBar);
    leftColumn->addWidget(mDeleteButton);
    leftColumn->addStretch(1);

    auto *rightColumn = new QVBoxLayout();
    rightColumn->setSpacing(12);

    auto *subtaskHeader = new QLabel(tr("Subtasks"), this);
    subtaskHeader->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: bold;"));
    rightColumn->addWidget(subtaskHeader);

    auto *subtaskContainer = new QWidget(this);
    mSubtaskLayout = new QVBoxLayout(subtaskContainer);
    mSubtaskLayout->setSpacing(12);
    mSubtaskLayout->setContentsMargins(0, 0, 0, 0);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(subtaskContainer);

    auto *addSubtaskButton = new QPushButton(tr("New Subtask"), this);
    addSubtaskButton->setCursor(Qt::PointingHandCursor);
    addSubtaskButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    addSubtaskButton->setMinimumHeight(48);
    addSubtaskButton->setStyleSheet(QStringLiteral(
        "QPushButton { border: 1px dashed #E0E0E0; background: transparent; border-radius: 14px; font-weight: bold; }"
        "QPushButton:hover { background: #F5F5F5; }"));

    rightColumn->addWidget(scroll, 1);
    rightColumn->addWidget(addSubtaskButton);

    layout->addLayout(leftColumn, 2);
    layout->addLayout(rightColumn, 3);

    connect(mTitleEdit, &QLineEdit::editingFinished, this, [this]() {
        const QString trimmed = mTitleEdit->text().trimmed();
        if (trimmed.isEmpty())
        {
            QMessageBox::warning(this, tr("Invalid Title"), tr("Task title cannot be empty."));
            const QSignalBlocker blocker(mTitleEdit);
            mTitleEdit->setText(mTask.title);
            return;
        }
        if (trimmed == mTask.title)
        {
            return;
        }
        mTask.title = trimmed;
        mTitleEdit->setText(trimmed);
        emit taskUpdated(mTask);
    });
    connect(mDescriptionEdit, &QTextEdit::textChanged, this, [this]() {
        mTask.description = mDescriptionEdit->toPlainText();
        emit taskUpdated(mTask);
    });
    connect(mStartEdit, &QDateTimeEdit::dateTimeChanged, this, [this](const QDateTime &dt) {
        if (dt >= mTask.endTime)
        {
            QMessageBox::warning(this, tr("Invalid Range"), tr("Start time must be before the end time."));
            const QSignalBlocker blocker(mStartEdit);
            mStartEdit->setDateTime(mTask.startTime);
            return;
        }
        if (dt < QDateTime::currentDateTime())
        {
            QMessageBox::warning(this, tr("Invalid Start"), tr("Start time cannot be in the past."));
            const QSignalBlocker blocker(mStartEdit);
            mStartEdit->setDateTime(mTask.startTime);
            return;
        }
        if (dt == mTask.startTime)
        {
            return;
        }
        mTask.startTime = dt;
        emit taskUpdated(mTask);
    });
    connect(mEndEdit, &QDateTimeEdit::dateTimeChanged, this, [this](const QDateTime &dt) {
        if (dt <= mTask.startTime)
        {
            QMessageBox::warning(this, tr("Invalid Range"), tr("End time must be after the start time."));
            const QSignalBlocker blocker(mEndEdit);
            mEndEdit->setDateTime(mTask.endTime);
            return;
        }
        if (dt <= QDateTime::currentDateTime())
        {
            QMessageBox::warning(this, tr("Invalid End"), tr("End time must be in the future."));
            const QSignalBlocker blocker(mEndEdit);
            mEndEdit->setDateTime(mTask.endTime);
            return;
        }
        if (dt == mTask.endTime)
        {
            return;
        }
        mTask.endTime = dt;
        emit taskUpdated(mTask);
    });
    connect(mDeleteButton, &QPushButton::clicked, this, [this]() {
        if (QMessageBox::question(this, tr("Delete Task"), tr("Are you sure you want to delete this task and all its subtasks?")) == QMessageBox::Yes)
        {
            emit taskDeleted(mTask.id);
        }
    });
    connect(addSubtaskButton, &QPushButton::clicked, this, [this]() {
        Subtask subtask;
        subtask.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        subtask.dueTime = QDateTime::currentDateTime().addSecs(3600);
        subtask.weighting = 1.0;
        mTask.subtasks.append(subtask);
        rebuildSubtasks();
        emit taskUpdated(mTask);
    });
}

void TaskDetailView::setTask(const Task &task)
{
    mTask = task;
    mTitleEdit->setText(task.title);
    mDescriptionEdit->setText(task.description);
    mStartEdit->setDateTime(task.startTime);
    mEndEdit->setDateTime(task.endTime);
    recalculateProgress();
    rebuildSubtasks();
}

void TaskDetailView::rebuildSubtasks()
{
    if (!mSubtaskLayout)
    {
        return;
    }

    QLayoutItem *item;
    while ((item = mSubtaskLayout->takeAt(0)) != nullptr)
    {
        if (item->widget())
        {
            item->widget()->deleteLater();
        }
        delete item;
    }

    if (mTask.subtasks.isEmpty())
    {
        auto *placeholder = new QLabel(tr("No subtasks yet. Click 'New Subtask' to add one."), this);
        placeholder->setStyleSheet(QStringLiteral("color: #888888;"));
        placeholder->setWordWrap(true);
        mSubtaskLayout->addWidget(placeholder);
    }
    else
    {
        for (auto &subtask : mTask.subtasks)
        {
            auto *row = new SubtaskRowWidget(this);
            row->setSubtask(subtask);
            connect(row, &SubtaskRowWidget::subtaskChanged, this, [this](const Subtask &updated) {
                for (auto &existing : mTask.subtasks)
                {
                    if (existing.id == updated.id)
                    {
                        existing = updated;
                        break;
                    }
                }
                recalculateProgress();
                emit taskUpdated(mTask);
            });
            connect(row, &SubtaskRowWidget::deleteRequested, this, [this](const QString &id) {
                mTask.subtasks.erase(std::remove_if(mTask.subtasks.begin(), mTask.subtasks.end(), [&](const Subtask &subtask) {
                                          return subtask.id == id;
                                      }),
                                      mTask.subtasks.end());
                rebuildSubtasks();
                recalculateProgress();
                emit taskUpdated(mTask);
            });
            mSubtaskLayout->addWidget(row);
        }
    }
    mSubtaskLayout->addStretch(1);
}

void TaskDetailView::recalculateProgress()
{
    const double progress = computeProgress(mTask);
    mProgressBar->setValue(static_cast<int>(std::round(progress)));
    mProgressBar->setFormat(tr("%1% completed").arg(QString::number(progress, 'f', 1)));
}

TasksPage::TasksPage(QWidget *parent)
    : QWidget(parent)
{
    createLayout();
}

void TasksPage::setJsonManager(JsonManager *manager)
{
    mJsonManager = manager;
}

void TasksPage::setTasks(const QVector<Task> &tasks)
{
    mTasks = tasks;
    rebuildList();
}

void TasksPage::refreshFromHome(const QVector<Activity> &activities)
{
    Q_UNUSED(activities);
}

void TasksPage::createLayout()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);

    auto *title = new QLabel(tr("Tasks"), this);
    title->setStyleSheet(QStringLiteral("font-size: 24px; font-weight: bold;"));

    auto *addButton = new QPushButton(tr("+"), this);
    addButton->setFixedSize(40, 40);
    addButton->setStyleSheet(QStringLiteral("QPushButton { border-radius: 20px; background: #000000; color: #FFFFFF; font-size: 20px; }") );
    addButton->setCursor(Qt::PointingHandCursor);

    headerLayout->addWidget(title);
    headerLayout->addStretch(1);
    headerLayout->addWidget(addButton);

    layout->addLayout(headerLayout);

    auto *divider = new QFrame(this);
    divider->setFrameShape(QFrame::HLine);
    divider->setFrameShadow(QFrame::Plain);
    divider->setStyleSheet(QStringLiteral("QFrame { background: #E0E0E0; max-height: 1px; min-height: 1px; }"));
    layout->addWidget(divider);

    mStack = new QStackedWidget(this);
    layout->addWidget(mStack, 1);

    mListPage = new QWidget(this);
    auto *listLayout = new QVBoxLayout(mListPage);
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->setSpacing(16);

    auto *scroll = new QScrollArea(mListPage);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *scrollContent = new QWidget(scroll);
    mListLayout = new QVBoxLayout(scrollContent);
    mListLayout->setSpacing(16);
    mListLayout->setContentsMargins(0, 0, 0, 0);

    scroll->setWidget(scrollContent);
    listLayout->addWidget(scroll);

    mStack->addWidget(mListPage);

    mDetailPage = new TaskDetailView(this);
    mStack->addWidget(mDetailPage);

    connect(addButton, &QPushButton::clicked, this, [this]() {
        TaskDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted)
        {
            Task task = dialog.task();
            if (task.id.isEmpty())
            {
                task.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            }
            mTasks.append(task);
            rebuildList();
            emit tasksChanged(mTasks);
        }
    });

    connect(mDetailPage, &TaskDetailView::taskUpdated, this, [this](const Task &task) {
        saveTask(task);
    });
    connect(mDetailPage, &TaskDetailView::taskDeleted, this, [this](const QString &id) {
        deleteTask(id);
    });
}

void TasksPage::rebuildList()
{
    if (!mListLayout)
    {
        return;
    }

    QLayoutItem *item;
    while ((item = mListLayout->takeAt(0)) != nullptr)
    {
        if (item->widget())
        {
            item->widget()->deleteLater();
        }
        delete item;
    }

    if (mTasks.isEmpty())
    {
        auto *placeholder = new QLabel(tr("No tasks yet. Click + to create your first task."), this);
        placeholder->setStyleSheet(QStringLiteral("color: #888888;"));
        placeholder->setAlignment(Qt::AlignCenter);
        placeholder->setWordWrap(true);
        mListLayout->addWidget(placeholder);
    }
    else
    {
        for (const auto &task : mTasks)
        {
            auto *card = new TaskCardWidget(this);
            card->setTask(task, computeProgress(task));
            connect(card, &TaskCardWidget::clicked, this, &TasksPage::openTaskDetail);
            mListLayout->addWidget(card);
        }
    }
    mListLayout->addStretch(1);
    mStack->setCurrentWidget(mListPage);
}

void TasksPage::openTaskDetail(const QString &taskId)
{
    auto it = std::find_if(mTasks.begin(), mTasks.end(), [&](const Task &task) {
        return task.id == taskId;
    });
    if (it == mTasks.end())
    {
        return;
    }
    mDetailPage->setTask(*it);
    mStack->setCurrentWidget(mDetailPage);
}

void TasksPage::saveTask(const Task &task)
{
    for (auto &existing : mTasks)
    {
        if (existing.id == task.id)
        {
            existing = task;
            rebuildList();
            emit tasksChanged(mTasks);
            return;
        }
    }
}

void TasksPage::deleteTask(const QString &taskId)
{
    mTasks.erase(std::remove_if(mTasks.begin(), mTasks.end(), [&](const Task &task) {
                      return task.id == taskId;
                  }),
                  mTasks.end());
    rebuildList();
    emit tasksChanged(mTasks);
}

double TasksPage::taskProgress(const Task &task) const
{
    return computeProgress(task);
}

void TasksPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (mStack && mListPage)
    {
        mStack->setCurrentWidget(mListPage);
    }
}
