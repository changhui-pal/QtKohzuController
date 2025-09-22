#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <vector>

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
    AxisControlWidget *axisWidget = new AxisControlWidget(this);
    axisWidget->setAxisNumber(nextAxisNumber_);

    connect(axisWidget, &AxisControlWidget::moveRequested, this, &MainWindow::handleMoveRequest);
    connect(axisWidget, &AxisControlWidget::removalRequested, this, &MainWindow::handleRemovalRequest);

    ui->axisLayout->addWidget(axisWidget);
    axisWidgets_.insert(nextAxisNumber_, axisWidget);
    nextAxisNumber_++;
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

        std::vector<int> axes;
        for(int axisNum : axisWidgets_.keys()) {
            axes.push_back(axisNum);
        }
        if (!axes.empty()) {
            manager_->startMonitoring(axes);
        }

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
        widget->deleteLater();
    }
}
