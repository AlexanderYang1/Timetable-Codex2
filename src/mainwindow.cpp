#include "mainwindow.h"

#include <QApplication>
#include <QBoxLayout>
#include <QEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QScreen>
#include <QTimer>

#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Timetable & Task Manager"));
    resize(1280, 720);
    setMouseTracking(true);
    centralWidget();
    createLayout();
    connectSignals();
    mJsonManager.ensureDataFiles();
    updateSidebarWidth();

    // Load data into pages
    const auto activities = mJsonManager.loadActivities();
    const auto tasks = mJsonManager.loadTasks();
    const auto settings = mJsonManager.loadSettings();
    const auto schoolPeriods = mJsonManager.loadSchoolPeriods();

    mHomePage->setJsonManager(&mJsonManager);
    mHomePage->setActivities(activities);
    mHomePage->setSchoolPeriods(schoolPeriods);
    mHomePage->setSettings(settings);

    mTimetablePage->setJsonManager(&mJsonManager);
    mTimetablePage->setSchoolPeriods(schoolPeriods);
    mTimetablePage->setSettings(settings);

    mTasksPage->setJsonManager(&mJsonManager);
    mTasksPage->setTasks(tasks);

    mSettingsPage->setJsonManager(&mJsonManager);
    mSettingsPage->setSettings(settings);

    installEventFilter(this);
    navigateTo(0);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseMove)
    {
        const auto mouseEvent = static_cast<QMouseEvent *>(event);
        const QPoint pos = mouseEvent->globalPosition().toPoint();
        const QPoint local = mapFromGlobal(pos);

        if (local.x() <= 20)
        {
            mSidebar->expand();
        }
        else if (!mSidebar->rect().contains(mSidebar->mapFrom(this, local)))
        {
            mSidebar->collapse();
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateSidebarWidth();
}

void MainWindow::createLayout()
{
    auto *central = new QWidget(this);
    auto *layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    mSidebar = new Sidebar(central);
    mStack = new QStackedWidget(central);

    layout->addWidget(mSidebar);
    layout->addWidget(mStack, 1);

    setCentralWidget(central);

    mHomePage = new HomePage(this);
    mTimetablePage = new TimetablePage(this);
    mTasksPage = new TasksPage(this);
    mSettingsPage = new SettingsPage(this);

    mStack->addWidget(mHomePage);
    mStack->addWidget(mTimetablePage);
    mStack->addWidget(mTasksPage);
    mStack->addWidget(mSettingsPage);
}

void MainWindow::connectSignals()
{
    connect(mSidebar, &Sidebar::pageRequested, this, &MainWindow::navigateTo);
    connect(mSettingsPage, &SettingsPage::settingsChanged, this, [this](const SettingsData &settings) {
        mJsonManager.saveSettings(settings);
        mHomePage->setSettings(settings);
        mTimetablePage->setSettings(settings);
    });

    connect(mHomePage, &HomePage::activitiesChanged, this, [this](const QVector<Activity> &activities) {
        mJsonManager.saveActivities(activities);
        mTasksPage->refreshFromHome(activities);
    });

    connect(mTasksPage, &TasksPage::tasksChanged, this, [this](const QVector<Task> &tasks) {
        mJsonManager.saveTasks(tasks);
    });
}

void MainWindow::updateSidebarWidth()
{
    const int width = std::max(220, width() / 6);
    mSidebar->setExpandedWidth(width);
    if (mSidebar->isExpanded())
    {
        mSidebar->setMaximumWidth(width);
        mSidebar->setMinimumWidth(width);
    }
}

void MainWindow::navigateTo(int index)
{
    if (index < 0 || index >= mStack->count())
    {
        return;
    }
    mStack->setCurrentIndex(index);
}
