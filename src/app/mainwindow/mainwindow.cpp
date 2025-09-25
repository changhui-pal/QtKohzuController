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
    // BUG FIX: ccw 버튼 클릭 시 입력 값을 음수로 변환
    if (is_ccw) {
        value_physical = -value_physical;
    }

    bool isAbsolute = widget->isAbsoluteMode();
    int speed = widget->getSelectedSpeed();

    double target_pos_physical = 0.0;
    if (isAbsolute) {
        target_pos_physical = value_physical;
    } else { // Relative
        int current_pulse = currentPositions_pulse_.value(axis, 0);
        double current_pos_physical = current_pulse * motor.value_per_pulse;
        target_pos_physical = current_pos_physical + value_physical;
    }

    // 유효성 검사
    if (qAbs(target_pos_physical) > motor.travel_range + 1e-9) {
        QMessageBox::critical(this, "Out of Range",
                              QString("Target position %1 %2 is out of range (± %3 %2).")
                                  .arg(target_pos_physical, 0, 'f', motor.display_precision)
                                  .arg(motor.unit_symbol)
                                  .arg(motor.travel_range));
        return;
    }

    // 물리 단위를 pulse로 변환
    int move_pulse = 0;
    if (isAbsolute) {
        // 절대 이동은 목표 위치를 펄스로 변환
        move_pulse = std::round(target_pos_physical / motor.value_per_pulse);
    } else {
        // 상대 이동은 이동할 거리만 펄스로 변환
        move_pulse = std::round(value_physical / motor.value_per_pulse);
    }

    manager_->move(axis, move_pulse, speed, isAbsolute);
}

void MainWindow::handleMotorSelectionChange(int axis, const QString &motorName)
{
    AxisControlWidget* widget = axisWidgets_.value(axis, nullptr);
    if (widget && motorDefinitions_.contains(motorName)) {
        const StageMotorInfo& motor = motorDefinitions_[motorName];
        widget->updateUiForMotor(motor);
        // 현재 위치(pulse)를 새 단위에 맞게 다시 계산하여 표시
        updatePosition(axis, currentPositions_pulse_.value(axis, 0));
    }
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
