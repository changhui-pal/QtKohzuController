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
    connect(widget, &AxisControlWidget::removalRequested, this, &MainWindow::handleRemovalRequest);
    connect(widget, &AxisControlWidget::motorSelectionChanged, this, &MainWindow::handleMotorSelectionChange);

    // 초기 모터 선택에 대한 UI 업데이트 강제 실행
    handleMotorSelectionChange(widget->getAxisNumber(), widget->getSelectedMotorName());
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

void MainWindow::updatePosition(int axis, int position_pulse)
{
    currentPositions_pulse_[axis] = position_pulse;
    AxisControlWidget* widget = axisWidgets_.value(axis, nullptr);
    if (widget) {
        QString motorName = widget->getSelectedMotorName();
        const StageMotorInfo& motor = motorDefinitions_[motorName];
        double position_mm = (motor.pulse_per_mm > 0) ? (position_pulse / motor.pulse_per_mm) : 0;
        widget->setPosition(position_mm);
    }
}

void MainWindow::handleMoveRequest(int axis)
{
    AxisControlWidget* widget = axisWidgets_.value(axis, nullptr);
    if (!widget) return;

    QString motorName = widget->getSelectedMotorName();
    const StageMotorInfo& motor = motorDefinitions_[motorName];

    double value_mm = widget->getInputValue();
    bool isAbsolute = widget->isAbsoluteMode();
    int speed = widget->getSelectedSpeed();

    // 버튼 방향에 따라 값 보정
    if (sender()->objectName() == "ccwButton") {
        value_mm = qAbs(value_mm * -1);
    } else {
        value_mm = qAbs(value_mm);
    }

    double target_pos_mm = 0.0;
    if (isAbsolute) {
        target_pos_mm = value_mm;
    } else { // Relative
        int current_pulse = currentPositions_pulse_.value(axis, 0);
        double current_pos_mm = (motor.pulse_per_mm > 0) ? (current_pulse / motor.pulse_per_mm) : 0;
        target_pos_mm = current_pos_mm + value_mm;
    }

    // 유효성 검사
    if (qAbs(target_pos_mm) > motor.travel_range_mm) {
        QMessageBox::critical(this, "Out of Range",
                              QString("Target position %1 mm is out of range (± %2 mm).")
                                  .arg(target_pos_mm).arg(motor.travel_range_mm));
        return;
    }

    // mm를 pulse로 변환
    int target_pulse = 0;
    if (isAbsolute) {
        target_pulse = std::round(value_mm * motor.pulse_per_mm);
    } else {
        target_pulse = std::round(value_mm * motor.pulse_per_mm);
    }

    manager_->move(axis, target_pulse, speed, isAbsolute);
}

void MainWindow::handleMotorSelectionChange(int axis, const QString &motorName)
{
    AxisControlWidget* widget = axisWidgets_.value(axis, nullptr);
    if (widget && motorDefinitions_.contains(motorName)) {
        widget->setTravelRange(motorDefinitions_[motorName].travel_range_mm);
        // 현재 위치(pulse)를 새 단위에 맞게 다시 계산하여 표시
        updatePosition(axis, currentPositions_pulse_.value(axis, 0));
    }
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
