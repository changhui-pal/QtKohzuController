#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include "QtKohzuManager.h"
#include "AxisControlWidget.h"

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
    // UI events
    void on_connectButton_clicked();
    void on_addAxisButton_clicked();

    // QtKohzuManager signals
    void logMessage(const QString &message);
    void updateConnectionStatus(bool connected);
    void updatePosition(int axis, int position);

    // AxisControlWidget signals
    void handleMoveRequest(int axis, int value, bool isAbsolute);
    void handleRemovalRequest(int axis);

private:
    Ui::MainWindow *ui;
    QtKohzuManager *manager_;

    QMap<int, AxisControlWidget*> axisWidgets_;
    int nextAxisNumber_ = 1;
};
#endif // MAINWINDOW_H
