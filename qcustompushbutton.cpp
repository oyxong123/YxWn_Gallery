#include "qcustompushbutton.h"

QCustomPushButton::QCustomPushButton(QWidget *parent) : QPushButton(parent) {
    QObject::connect(this, &QPushButton::pressed, this, &QCustomPushButton::btnPressed);
    QObject::connect(this, &QPushButton::released, this, &QCustomPushButton::btnReleased);
}

void QCustomPushButton::setIcons(const QIcon &defaultIcon, const QIcon &hoverIcon, const QIcon &clickedIcon) {
    this->defaultIcon = defaultIcon;
    this->hoverIcon = hoverIcon;
    this->clickedIcon = clickedIcon;
    setIcon(defaultIcon);
    setIconSize(size());  // Size only need to set once, it will retain when image changes.
}

void QCustomPushButton::enterEvent(QEnterEvent *event) {
    setIcon(hoverIcon);
    QPushButton::enterEvent(event);
}

void QCustomPushButton::leaveEvent(QEvent *event) {
    setIcon(defaultIcon);
    QPushButton::leaveEvent(event);
}

void QCustomPushButton::btnPressed() {
    setIcon(clickedIcon);
}

void QCustomPushButton::btnReleased() {
    setIcon(hoverIcon);
}
