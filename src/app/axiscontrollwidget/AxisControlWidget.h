#ifndef AXISCONTROLWIDGET_H
#define AXISCONTROLWIDGET_H

#include <QWidget>

namespace Ui {
class AxisControlWidget;
}

class AxisControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AxisControlWidget(QWidget *parent = nullptr);
    ~AxisControlWidget();

    int getAxisNumber() const;
    void setAxisNumber(int axisNumber); // 축 번호를 설정하는 함수

public slots:
    void setPosition(int position);

signals:
    void moveRequested(int axis, int value, bool isAbsolute);
    void removalRequested(int axis);

private slots:
    void on_cwButton_clicked();
    void on_ccwButton_clicked();
    void on_removeButton_clicked();

private:
    Ui::AxisControlWidget *ui;
    int currentAxisNumber_ = 0; // 현재 축 번호를 저장할 멤버 변수
};

#endif // AXISCONTROLWIDGET_H
