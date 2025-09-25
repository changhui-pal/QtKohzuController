#ifndef AXISCONTROLWIDGET_H
#define AXISCONTROLWIDGET_H

#include <QWidget>
#include "StageMotorInfo.h"

namespace Ui {
class AxisControlWidget;
}

class AxisControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AxisControlWidget(QWidget *parent = nullptr);
    ~AxisControlWidget();

    // Getters
    int getAxisNumber() const;
    QString getSelectedMotorName() const;
    double getInputValue() const;
    int getSelectedSpeed() const;
    bool isAbsoluteMode() const;

    // UI Update & Setup
    void setAxisNumber(int axisNumber);
    void populateMotorDropdown(const QMap<QString, StageMotorInfo>& motors);
    void setPosition(double physical_position);
    void updateUiForMotor(const StageMotorInfo& motor);

signals:
    void moveRequested(int axis, bool is_ccw); // cw/ccw 여부를 bool로 전달
    void removalRequested(int axis);
    void motorSelectionChanged(int axis, const QString& motorName);

private slots:
    void on_removeButton_clicked();
    void on_cwButton_clicked();
    void on_ccwButton_clicked();
    void on_motorComboBox_currentIndexChanged(const QString &arg1);

private:
    Ui::AxisControlWidget *ui;
    int currentAxisNumber_ = 0;
    int displayPrecision_ = 4; // 현재 모터의 표시 정밀도
};

#endif // AXISCONTROLWIDGET_H
