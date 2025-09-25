#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include "QtKohzuManager.h"
#include "AxisControlWidget.h"
#include "StageMotorInfo.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_connectButton_clicked();
    void on_addAxisButton_clicked();

    void logMessage(const QString &message);
    void updateConnectionStatus(bool connected);
    void updatePosition(int axis, int position_pulse);

    void handleMoveRequest(int axis, bool is_ccw);
    void handleRemovalRequest(int axis);
    void handleMotorSelectionChange(int axis, const QString& motorName);

private:
    void restartMonitoring();
    void setupAxisWidget(AxisControlWidget* widget);

    Ui::MainWindow *ui;
    QtKohzuManager *manager_;

    QMap<int, AxisControlWidget*> axisWidgets_;
    QMap<QString, StageMotorInfo> motorDefinitions_;
    QMap<int, int> currentPositions_pulse_;
};
#endif // MAINWINDOW_H

