#include "sidebar.h"

#include <QBoxLayout>
#include <QEvent>
#include <QGraphicsDropShadowEffect>

namespace
{
struct SidebarItem
{
    QString text;
};

const QList<SidebarItem> kItems = {
    {QStringLiteral("Homepage")},
    {QStringLiteral("Timetable")},
    {QStringLiteral("Tasks")},
    {QStringLiteral("Settings")}};

constexpr int kAnimationDurationMs = 220;
}

Sidebar::Sidebar(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("Sidebar"));
    setFrameShape(QFrame::NoFrame);
    setAutoFillBackground(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(16.0);
    shadow->setOffset(4.0, 0.0);
    shadow->setColor(QColor(0, 0, 0, 30));
    setGraphicsEffect(shadow);

    setStyleSheet(QStringLiteral("#Sidebar { background: #FFFFFF; border-right: 1px solid #E0E0E0; }\n"
                                 "#Sidebar QToolButton { text-align: left; padding: 12px 16px; font-size: 15px; border: none; }\n"
                                 "#Sidebar QToolButton:hover { background: #F5F5F5; }\n"
                                 "#Sidebar QToolButton:checked { background: #E0E0E0; font-weight: bold; }");

    mAnimation = new QPropertyAnimation(this, "maximumWidth", this);
    mAnimation->setDuration(kAnimationDurationMs);
    mAnimation->setEasingCurve(QEasingCurve::InOutQuad);
    setMaximumWidth(0);
    setMinimumWidth(0);

    rebuildUi();
}

void Sidebar::setExpandedWidth(int width)
{
    mExpandedWidth = width;
}

int Sidebar::expandedWidth() const
{
    return mExpandedWidth;
}

void Sidebar::expand()
{
    if (isExpanded())
    {
        return;
    }
    animateToWidth(mExpandedWidth);
}

void Sidebar::collapse()
{
    if (!isExpanded())
    {
        return;
    }
    animateToWidth(0);
}

bool Sidebar::isExpanded() const
{
    return maximumWidth() > 0;
}

void Sidebar::enterEvent(QEvent *event)
{
    mExplicitHover = true;
    expand();
    QFrame::enterEvent(event);
}

void Sidebar::leaveEvent(QEvent *event)
{
    mExplicitHover = false;
    collapse();
    QFrame::leaveEvent(event);
}

void Sidebar::rebuildUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 24, 0, 24);
    layout->setSpacing(8);

    int index = 0;
    for (const auto &item : kItems)
    {
        auto *button = new QToolButton(this);
        button->setText(item.text);
        button->setCheckable(true);
        button->setAutoExclusive(true);
        button->setCursor(Qt::PointingHandCursor);
        connect(button, &QToolButton::clicked, this, [this, index]() {
            emit pageRequested(index);
        });

        mButtons.append(button);
        layout->addWidget(button);
        ++index;
    }

    layout->addStretch(1);
}

void Sidebar::animateToWidth(int width)
{
    if (mAnimation->state() == QAbstractAnimation::Running)
    {
        mAnimation->stop();
    }

    mAnimation->setStartValue(maximumWidth());
    mAnimation->setEndValue(width);
    mAnimation->start();
    setMinimumWidth(width);
}
