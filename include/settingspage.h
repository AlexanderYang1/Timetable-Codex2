#pragma once

#include "jsonmanager.h"
#include "models.h"

#include <QButtonGroup>
#include <QWidget>

class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(QWidget *parent = nullptr);

    void setJsonManager(JsonManager *manager);
    void setSettings(const SettingsData &settings);

signals:
    void settingsChanged(const SettingsData &settings);

private:
    void createLayout();
    void updateControls();

    JsonManager *mJsonManager = nullptr;
    SettingsData mSettings;

    QButtonGroup *mWeekGroup = nullptr;
    QButtonGroup *mYearGroup = nullptr;
};
