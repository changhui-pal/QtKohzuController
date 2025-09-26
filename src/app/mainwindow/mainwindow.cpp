#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <vector>
#include <QMessageBox>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    manager_ = new QtKohzuManager(this);
    motorDefinitions_ = getMotorDefinitions(); // 모터 정보 로드

    connect(manager_, &QtKohzuManager::logMessage, this, &MainWindow::logMessage);
    connect(manager_, &QtKohzuManager::connectionStatusChanged, this, &MainWindow::updateConnectionStatus);
    connect(manager_, &QtKohzuManager::positionUpdated, this, &MainWindow::updatePosition);

    updateConnectionStatus(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupAxisWidget(AxisControlWidget* widget)
{
    widget->populateMotorDropdown(motorDefinitions_);
    connect(widget, &AxisControlWidget::moveRequested, this, &MainWindow::handleMoveRequest);
    connect(widget, &AxisControlWidget::originRequested, this, &MainWindow::handleOriginRequest);
    connect(widget, &AxisControlWidget::removalRequested, this, &MainWindow::handleRemovalRequest);
    connect(widget, &AxisControlWidget::motorSelectionChanged, this, &MainWindow::handleMotorSelectionChange);
}

void MainWindow::on_connectButton_clicked()
{
    if (ui->connectButton->text() == "Connect") {
        manager_->connectToController(ui->ipAddressEdit->text(), ui->portEdit->text().toUShort());
    } else {
        manager_->disconnectFromController();
    }
}

void MainWindow::on_addAxisButton_clicked()
{
    int axisToAdd = ui->addAxisSpinBox->value();
    if (axisWidgets_.contains(axisToAdd)) {
        QMessageBox::warning(this, "Duplicate Axis", QString("Axis %1 already exists.").arg(axisToAdd));
        return;
    }
    AxisControlWidget *axisWidget = new AxisControlWidget(this);
    axisWidget->setAxisNumber(axisToAdd);

    // =======================================================================
    // BUG FIX: 위젯을 맵에 먼저 추가한 후, UI 설정을 진행합니다.
    // =======================================================================
    axisWidgets_.insert(axisToAdd, axisWidget);
    currentPositions_pulse_[axisToAdd] = 0; // 새 축 위치 캐시 초기화

    // 이제 위젯이 맵에 존재하므로, setup 함수가 정상적으로 동작합니다.
    setupAxisWidget(axisWidget);

    ui->axisLayout->addWidget(axisWidget);

    restartMonitoring();

    // 위젯이 최초로 생성되면 원점 복귀 8번(ccw limit을 원점) 설정 후 원점으로 이동
    manager_->setSystem(axisToAdd, 2, 8);
    manager_->moveOrigin(axisToAdd, 2);
}

void MainWindow::logMessage(const QString &message)
{
    ui->logTextEdit->append(message);
}

void MainWindow::updateConnectionStatus(bool connected) {
    ui->connectionGroup->setEnabled(!connected);
    ui->controlGroup->setEnabled(connected);
    if (connected) {
        ui->connectButton->setText("Disconnect");
        restartMonitoring();
    } else {
        ui->connectButton->setText("Connect");
        if(manager_) manager_->stopMonitoring();
    }
}

void MainWindow::updatePosition(int axis, int position_pulse)
{
    currentPositions_pulse_[axis] = position_pulse;
    AxisControlWidget* widget = axisWidgets_.value(axis, nullptr);
    if (widget) {
        QString motorName = widget->getSelectedMotorName();
        const StageMotorInfo& motor = motorDefinitions_[motorName];
        // 정밀도 손실 없이 double 타입으로 물리적 위치를 계산합니다.
        double position_physical = static_cast<double>(position_pulse) * motor.value_per_pulse;
        widget->setPosition(position_physical);
    }
}

void MainWindow::handleMoveRequest(int axis, bool is_ccw)
{
    AxisControlWidget* widget = axisWidgets_.value(axis, nullptr);
    if (!widget) return;

    QString motorName = widget->getSelectedMotorName();
    const StageMotorInfo& motor = motorDefinitions_[motorName];

    double value_physical = widget->getInputValue();
    if (is_ccw) {
        value_physical = -qAbs(value_physical);
    } else {
        value_physical = qAbs(value_physical);
    }

    bool isAbsolute = widget->isAbsoluteMode();
    int speed = widget->getSelectedSpeed();

    double target_pos_physical = 0.0;
    if (isAbsolute) {
        target_pos_physical = value_physical;
    } else {
        int current_pulse = currentPositions_pulse_.value(axis, 0);
        double current_pos_physical = static_cast<double>(current_pulse) * motor.value_per_pulse;
        target_pos_physical = current_pos_physical + value_physical;
    }

    if (qAbs(target_pos_physical) > motor.travel_range + 1e-9) {
        QMessageBox::critical(this, "Out of Range",
                              QString("Target position %1 %2 is out of range (± %3 %2).")
                                  .arg(target_pos_physical, 0, 'f', motor.display_precision)
                                  .arg(motor.unit_symbol)
                                  .arg(motor.travel_range));
        return;
    }

    int move_pulse = 0;
    if (motor.value_per_pulse == 0) return; // 0으로 나누기 방지

    if (isAbsolute) {
        move_pulse = std::round(target_pos_physical / motor.value_per_pulse);
    } else {
        move_pulse = std::round(value_physical / motor.value_per_pulse);
    }

    manager_->move(axis, move_pulse, speed, isAbsolute);
}

void MainWindow::handleOriginRequest(int axis)
{
    AxisControlWidget* widget = axisWidgets_.value(axis, nullptr);
    if (!widget) return;

    int speed = widget->getSelectedSpeed();
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirm Origin Return",
                                  QString("Are you sure you want to perform an origin return for Axis %1?").arg(axis),
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        manager_->moveOrigin(axis, speed);
    }
}

void MainWindow::handleMotorSelectionChange(int axis, const QString &motorName)
{
    // 위젯이 자신의 UI를 업데이트 한 후, MainWindow는 데이터만 갱신합니다.
    // 현재 위치를 새로운 단위에 맞게 다시 계산하여 표시하도록 합니다.
    updatePosition(axis, currentPositions_pulse_.value(axis, 0));
}

void MainWindow::handleRemovalRequest(int axis) {
    if (axisWidgets_.contains(axis)) {
        AxisControlWidget *widget = axisWidgets_.take(axis);
        currentPositions_pulse_.remove(axis);
        widget->deleteLater();
        restartMonitoring();
    }
}

void MainWindow::restartMonitoring() {
    if (!manager_ || !ui->controlGroup->isEnabled()) return;
    manager_->stopMonitoring();
    QList<int> axisList = axisWidgets_.keys();
    if (!axisList.isEmpty()) {
        std::vector<int> axesToMonitor(axisList.begin(), axisList.end());
        manager_->startMonitoring(axesToMonitor);
    }
}
