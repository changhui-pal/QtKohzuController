#ifndef AXISCONTROLWIDGET_H
#define AXISCONTROLWIDGET_H

#include <QWidget>
#include <QMap>
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

signals:
    void moveRequested(int axis, bool is_ccw);
    void originRequested(int axis);
    void removalRequested(int axis);
    void motorSelectionChanged(int axis, const QString& motorName);

private slots:
    void on_removeButton_clicked();
    void on_cwButton_clicked();
    void on_ccwButton_clicked();
    void on_originButton_clicked();
    void on_motorComboBox_currentIndexChanged(int index);

private:
    void updateUiForMotor(const StageMotorInfo& motor);

    Ui::AxisControlWidget *ui;
    int currentAxisNumber_ = 0;
    int displayPrecision_ = 4;
    // 위젯이 모터 정보를 직접 소유하도록 변경
    QMap<QString, StageMotorInfo> motorDefinitions_;
};

#endif // AXISCONTROLWIDGET_H

