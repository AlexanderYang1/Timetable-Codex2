#pragma once

#include "homepage.h"
#include "jsonmanager.h"
#include "settingspage.h"
#include "sidebar.h"
#include "taskspage.h"
#include "timetablepage.h"

#include <QMainWindow>
#include <QStackedWidget>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void createLayout();
    void connectSignals();
    void updateSidebarWidth();
    void navigateTo(int index);

    JsonManager mJsonManager;
    Sidebar *mSidebar = nullptr;
    QStackedWidget *mStack = nullptr;
    HomePage *mHomePage = nullptr;
    TimetablePage *mTimetablePage = nullptr;
    TasksPage *mTasksPage = nullptr;
    SettingsPage *mSettingsPage = nullptr;
};
