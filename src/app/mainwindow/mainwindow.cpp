#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "PresetDialog.h"
#include <vector>
#include <QMessageBox>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    manager_ = new QtKohzuManager(this);
    presetManager_ = new PresetManager(this);
    motorDefinitions_ = getMotorDefinitions();

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
        QString host = ui->hostLineEdit->text();
        quint16 port = ui->portLineEdit->text().toUShort();
        manager_->connectToController(host, port);
    } else {
        manager_->disconnectFromController();
    }
}

void MainWindow::updateConnectionStatus(bool connected)
{
    // 그룹 전체 대신 개별 위젯을 제어하여 Disconnect 버튼을 항상 활성화합니다.
    ui->hostLineEdit->setEnabled(!connected);
    ui->portLineEdit->setEnabled(!connected);
    ui->controlGroup->setEnabled(connected);

    if (connected) {
        ui->connectButton->setText("Disconnect");
    } else {
        ui->connectButton->setText("Connect");
        if(manager_) manager_->stopMonitoring();
    }
}

void MainWindow::handleMoveRequest(int axis, bool is_ccw)
{
    AxisControlWidget* widget = axisWidgets_.value(axis, nullptr);
    if (!widget) return;
    savePreset(axis);

    QString motorName = widget->getSelectedMotorName();
    const StageMotorInfo& motor = motorDefinitions_[motorName];
    double value_physical = widget->getInputValue();
    if (is_ccw) { value_physical = -qAbs(value_physical); } else { value_physical = qAbs(value_physical); }
    bool isAbsolute = widget->isAbsoluteMode();
    int speed = widget->getSelectedSpeed();

    double target_pos_physical = 0.0;
    if (isAbsolute) {
        target_pos_physical = value_physical;
    } else {
        double current_pos_physical = static_cast<double>(currentPositions_pulse_.value(axis, 0)) * motor.value_per_pulse;
        target_pos_physical = current_pos_physical + value_physical;
    }

    // 가동 범위를 0 ~ (range * 2)로 검사합니다.
    double max_range = motor.travel_range * 2.0;
    if (target_pos_physical < (0.0 - 1e-9) || target_pos_physical > (max_range + 1e-9)) {
        QMessageBox::critical(this, "Out of Range",
                              QString("Target position %1 %2 is out of range (0 ~ %3 %2).")
                                  .arg(target_pos_physical, 0, 'f', motor.display_precision)
                                  .arg(motor.unit_symbol)
                                  .arg(max_range, 0, 'f', motor.display_precision));
        return;
    }

    if (motor.value_per_pulse == 0) return;

    int move_pulse = 0;
    if (isAbsolute) { move_pulse = std::round(target_pos_physical / motor.value_per_pulse); } else { move_pulse = std::round(value_physical / motor.value_per_pulse); }

    manager_->startMonitoring({axis});
    manager_->move(axis, move_pulse, speed, isAbsolute);
}


// ... (Other functions are unchanged)
void MainWindow::on_addAxisButton_clicked()
{
    int axisToAdd = ui->addAxisSpinBox->value();
    if (axisWidgets_.contains(axisToAdd)) {
        QMessageBox::warning(this, "Duplicate Axis", QString("Axis %1 already exists.").arg(axisToAdd));
        return;
    }

    AxisControlWidget *axisWidget = new AxisControlWidget(this);
    axisWidget->setAxisNumber(axisToAdd);
    axisWidgets_.insert(axisToAdd, axisWidget);
    currentPositions_pulse_[axisToAdd] = 0;
    setupAxisWidget(axisWidget);
    ui->axisLayout->addWidget(axisWidget);

    // 축 생성 시 원점 복귀 타입 설정 (ccw limit을 원점)
    manager_->setSystem(axisToAdd, 2, 8);
}

void MainWindow::handleOriginRequest(int axis)
{
    AxisControlWidget* widget = axisWidgets_.value(axis, nullptr);
    if (!widget) return;
    savePreset(axis);
    int speed = widget->getSelectedSpeed();

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Origin Return",
                                                              QString("Are you sure you want to perform an origin return for Axis %1?").arg(axis),
                                                              QMessageBox::Yes|QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // 원점 복귀 직전에 모니터링 시작
        manager_->startMonitoring({axis});
        manager_->moveOrigin(axis, speed);
    }
}

void MainWindow::handleRemovalRequest(int axis)
{
    if (axisWidgets_.contains(axis)) {
        AxisControlWidget *widget = axisWidgets_.take(axis);
        currentPositions_pulse_.remove(axis);
        widget->deleteLater();
        // 축 제거 시에는 모니터링을 재시작할 필요 없음
    }
}

void MainWindow::setupAxisWidget(AxisControlWidget* widget)
{
    widget->populateMotorDropdown(motorDefinitions_);
    connect(widget, &AxisControlWidget::moveRequested, this, &MainWindow::handleMoveRequest);
    connect(widget, &AxisControlWidget::originRequested, this, &MainWindow::handleOriginRequest);
    connect(widget, &AxisControlWidget::removalRequested, this, &MainWindow::handleRemovalRequest);
    connect(widget, &AxisControlWidget::motorSelectionChanged, this, &MainWindow::handleMotorSelectionChange);
    connect(widget, &AxisControlWidget::importRequested, this, &MainWindow::handleImportRequest);
}

void MainWindow::handleImportRequest(int axis)
{
    PresetDialog dialog(axis, presetManager_, this);
    connect(&dialog, &PresetDialog::presetApplied, this, [this, axis](const AxisPreset& preset){
        if (axisWidgets_.contains(axis)) {
            axisWidgets_[axis]->applyPreset(preset);
        }
    });
    dialog.exec();
}

void MainWindow::savePreset(int axis)
{
    AxisControlWidget* widget = axisWidgets_.value(axis, nullptr);
    if(!widget) return;
    AxisPreset preset;
    preset.id = QUuid::createUuid();
    preset.motorName = widget->getSelectedMotorName();
    preset.isAbsolute = widget->isAbsoluteMode();
    preset.value = widget->getInputValue();
    preset.speed = widget->getSelectedSpeed();
    presetManager_->addPreset(axis, preset);
}

void MainWindow::updatePosition(int axis, int position_pulse)
{
    currentPositions_pulse_[axis] = position_pulse;
    AxisControlWidget* widget = axisWidgets_.value(axis, nullptr);
    if (widget) {
        QString motorName = widget->getSelectedMotorName();
        if (motorDefinitions_.contains(motorName)) {
            const StageMotorInfo& motor = motorDefinitions_[motorName];
            double position_physical = static_cast<double>(position_pulse) * motor.value_per_pulse;
            widget->setPosition(position_physical);
        }
    }
}

void MainWindow::handleMotorSelectionChange(int axis, const QString &motorName)
{
    updatePosition(axis, currentPositions_pulse_.value(axis, 0));
}

void MainWindow::logMessage(const QString &message)
{
    ui->logTextEdit->append(message);
}

