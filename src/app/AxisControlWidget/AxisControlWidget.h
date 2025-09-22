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

public slots:
    void setPosition(int position);
    void setAxisNumber(int axisNumber);

signals:
    void moveRequested(int axis, int value, bool isAbsolute);
    void removalRequested(int axis);

private slots:
    void on_cwButton_clicked();
    void on_ccwButton_clicked();
    void on_removeButton_clicked();
    void on_axisSpinBox_valueChanged(int arg1);

private:
    Ui::AxisControlWidget *ui;
};

#endif // AXISCONTROLWIDGET_H
