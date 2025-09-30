#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "PresetDialog.h"
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
    ui->hostLineEdit->setEnabled(!connected);
    ui->portLineEdit->setEnabled(!connected);
    ui->controlGroup->setEnabled(connected);

    if (connected) {
        ui->connectButton->setText("Disconnect");
    } else {
        ui->connectButton->setText("Connect");
        if(manager_) manager_->clearPollAxes();
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
    axisWidgets_.insert(axisToAdd, axisWidget);
    currentPositionsPulse_.insert(axisToAdd, 0);
    setupAxisWidget(axisWidget);
    ui->axisLayout->addWidget(axisWidget);

    // Add axis to polling and background monitoring
    manager_->addAxisToPoll(axisToAdd);
    manager_->setSystem(axisToAdd, 2, 8);
}

void MainWindow::handleRemovalRequest(int axis)
{
    if (axisWidgets_.contains(axis)) {
        // Remove from polling and background monitoring
        manager_->removeAxisToPoll(axis);

        AxisControlWidget *widget = axisWidgets_.take(axis);
        currentPositionsPulse_.remove(axis);
        widget->deleteLater();
    }
}

void MainWindow::handleMoveRequest(int axis, bool isCcw)
{
    AxisControlWidget* widget = axisWidgets_.value(axis, nullptr);
    if (!widget) return;
    savePreset(axis);

    QString motorName = widget->getSelectedMotorName();
    const StageMotorInfo& motor = motorDefinitions_[motorName];
    double valuePhysical = widget->getInputValue();
    if (isCcw) { valuePhysical = -qAbs(valuePhysical); } else { valuePhysical = qAbs(valuePhysical); }
    bool isAbsolute = widget->isAbsoluteMode();
    int speed = widget->getSelectedSpeed();

    double targetPosPhysical = 0.0;
    if (isAbsolute) {
        targetPosPhysical = valuePhysical;
    } else {
        double currentPosPhysical = static_cast<double>(currentPositionsPulse_.value(axis, 0)) * motor.value_per_pulse;
        targetPosPhysical = currentPosPhysical + valuePhysical;
    }

    double maxRange = motor.travel_range * 2.0;
    if (targetPosPhysical < (0.0 - 1e-9) || targetPosPhysical > (maxRange + 1e-9)) {
        QMessageBox::critical(this, "Out of Range",
                              QString("Target position %1 %2 is out of range (0 ~ %3 %2).")
                                  .arg(targetPosPhysical, 0, 'f', motor.display_precision)
                                  .arg(motor.unit_symbol)
                                  .arg(maxRange, 0, 'f', motor.display_precision));
        return;
    }

    if (motor.value_per_pulse == 0) return;

    int movePulse = 0;
    if (isAbsolute) { movePulse = std::round(targetPosPhysical / motor.value_per_pulse); } else { movePulse = std::round(valuePhysical / motor.value_per_pulse); }

    manager_->move(axis, movePulse, speed, isAbsolute);
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
        manager_->moveOrigin(axis, speed);
    }
}

// ... (Other functions like setupAxisWidget, handleImportRequest, etc. are unchanged)
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

void MainWindow::updatePosition(int axis, int positionPulse)
{
    currentPositionsPulse_[axis] = positionPulse;
    AxisControlWidget* widget = axisWidgets_.value(axis, nullptr);
    if (widget) {
        QString motorName = widget->getSelectedMotorName();
        if (motorDefinitions_.contains(motorName)) {
            const StageMotorInfo& motor = motorDefinitions_[motorName];
            double positionPhysical = static_cast<double>(positionPulse) * motor.value_per_pulse;
            widget->setPosition(positionPhysical);
        }
    }
}

void MainWindow::handleMotorSelectionChange(int axis, const QString &motorName)
{
    updatePosition(axis, currentPositionsPulse_.value(axis, 0));
}

void MainWindow::logMessage(const QString &message)
{
    ui->logTextEdit->append(message);
}

