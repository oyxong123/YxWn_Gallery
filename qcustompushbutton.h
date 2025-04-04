#ifndef QCUSTOMPUSHBUTTON_H
#define QCUSTOMPUSHBUTTON_H

#include <QPushButton>

class QCustomPushButton : public QPushButton {
    Q_OBJECT

public:
    QCustomPushButton(QWidget *parent = nullptr);
    void setIcons(const QIcon &defaultIcon, const QIcon &hoverIcon = QIcon(), const QIcon &clickedIcon = QIcon());

private slots:
    void btnPressed();
    void btnReleased();

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QIcon defaultIcon;
    QIcon hoverIcon;
    QIcon clickedIcon;
};

#endif // QCUSTOMPUSHBUTTON_H
