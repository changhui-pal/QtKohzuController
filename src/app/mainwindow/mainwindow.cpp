#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <vector>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    manager_ = new QtKohzuManager(this);

    // Connect manager signals to MainWindow slots
    connect(manager_, &QtKohzuManager::logMessage, this, &MainWindow::logMessage);
    connect(manager_, &QtKohzuManager::connectionStatusChanged, this, &MainWindow::updateConnectionStatus);
    connect(manager_, &QtKohzuManager::positionUpdated, this, &MainWindow::updatePosition);

    updateConnectionStatus(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_connectButton_clicked()
{
    if (ui->connectButton->text() == "Connect") {
        manager_->connectToServer(ui->ipAddressEdit->text(), ui->portEdit->text());
    } else {
        manager_->disconnectFromServer();
    }
}

void MainWindow::on_addAxisButton_clicked()
{
    int axisToAdd = ui->addAxisSpinBox->value();
    if (axisWidgets_.contains(axisToAdd)) {
        QMessageBox::warning(this, "Duplicate Axis",
                             QString("Axis %1 already exists.").arg(axisToAdd));
        return;
    }

    AxisControlWidget *axisWidget = new AxisControlWidget(this);
    axisWidget->setAxisNumber(axisToAdd);

    connect(axisWidget, &AxisControlWidget::moveRequested, this, &MainWindow::handleMoveRequest);
    connect(axisWidget, &AxisControlWidget::removalRequested, this, &MainWindow::handleRemovalRequest);

    ui->axisLayout->addWidget(axisWidget);
    axisWidgets_.insert(axisToAdd, axisWidget);

    restartMonitoring();
}

void MainWindow::logMessage(const QString &message)
{
    ui->logTextEdit->append(message);
}

void MainWindow::updateConnectionStatus(bool connected)
{
    ui->connectionGroup->setEnabled(!connected);
    ui->controlGroup->setEnabled(connected);

    if (connected) {
        ui->connectButton->setText("Disconnect");
        restartMonitoring();
    } else {
        ui->connectButton->setText("Connect");
        manager_->stopMonitoring();
    }
}

void MainWindow::updatePosition(int axis, int position)
{
    if (axisWidgets_.contains(axis)) {
        axisWidgets_[axis]->setPosition(position);
    }
}

void MainWindow::handleMoveRequest(int axis, int value, bool isAbsolute)
{
    manager_->move(axis, value, isAbsolute);
}

void MainWindow::handleRemovalRequest(int axis)
{
    if (axisWidgets_.contains(axis)) {
        AxisControlWidget *widget = axisWidgets_.take(axis);
        ui->axisLayout->removeWidget(widget);
        widget->deleteLater(); // Defer deletion

        restartMonitoring();
    }
}

void MainWindow::restartMonitoring()
{
    // 연결되어 있지 않거나, 애플리케이션 종료 중이면 아무것도 하지 않음
    if (!manager_ || !ui->controlGroup->isEnabled()) {
        return;
    }

    manager_->stopMonitoring();

    // QMap의 key들로부터 모니터링할 축 목록을 가져옴
    QList<int> axisList = axisWidgets_.keys();
    if (!axisList.isEmpty()) {
        std::vector<int> axesToMonitor(axisList.begin(), axisList.end());
        manager_->startMonitoring(axesToMonitor);
    }
}
