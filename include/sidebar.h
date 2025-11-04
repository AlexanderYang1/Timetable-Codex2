#pragma once

#include <QFrame>
#include <QPropertyAnimation>
#include <QString>
#include <QToolButton>
#include <QVector>

class Sidebar : public QFrame
{
    Q_OBJECT
public:
    explicit Sidebar(QWidget *parent = nullptr);

    void setExpandedWidth(int width);
    int expandedWidth() const;

    void expand();
    void collapse();
    bool isExpanded() const;

signals:
    void pageRequested(int index);

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void rebuildUi();
    void animateToWidth(int width);

    QVector<QToolButton *> mButtons;
    QPropertyAnimation *mAnimation = nullptr;
    int mExpandedWidth = 260;
    bool mExplicitHover = false;
};
