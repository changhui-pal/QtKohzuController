#ifndef AXISCONTROLWIDGET_H
#define AXISCONTROLWIDGET_H

#include <QWidget>
#include "StageMotorInfo.h" // 모터 정보 구조체 헤더

namespace Ui {
class AxisControlWidget;
}

class AxisControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AxisControlWidget(QWidget *parent = nullptr);
    ~AxisControlWidget();

    // Getter 함수들
    int getAxisNumber() const;
    QString getSelectedMotorName() const;
    double getInputValue() const;
    int getSelectedSpeed() const;
    bool isAbsoluteMode() const;

    // Setter 및 설정 함수들
    void setAxisNumber(int axisNumber);
    void populateMotorDropdown(const QMap<QString, StageMotorInfo>& motors);
    void setTravelRange(double range);
    void setPosition(double position_mm);

signals:
    void moveRequested(int axis); // 이동 버튼 클릭 시 축 번호만 알림
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
};

#endif // AXISCONTROLWIDGET_H
