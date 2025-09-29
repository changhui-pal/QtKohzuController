#include "AxisControlWidget.h"
#include "ui_AxisControlWidget.h"
#include <QString>

AxisControlWidget::AxisControlWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AxisControlWidget)
{
    ui->setupUi(this);
    ui->absoluteRadioButton->setChecked(true);
    for (int i = 0; i <= 9; ++i) {
        ui->speedComboBox->addItem(QString::number(i));
    }
}

AxisControlWidget::~AxisControlWidget()
{
    delete ui;
}

int AxisControlWidget::getAxisNumber() const { return currentAxisNumber_; }
QString AxisControlWidget::getSelectedMotorName() const { return ui->motorComboBox->currentText(); }
double AxisControlWidget::getInputValue() const { return ui->valueLineEdit->text().toDouble(); }
int AxisControlWidget::getSelectedSpeed() const { return ui->speedComboBox->currentText().toInt(); }
bool AxisControlWidget::isAbsoluteMode() const { return ui->absoluteRadioButton->isChecked(); }

void AxisControlWidget::setAxisNumber(int axisNumber)
{
    currentAxisNumber_ = axisNumber;
    ui->container->setTitle(QString("Axis %1").arg(axisNumber));
}

void AxisControlWidget::populateMotorDropdown(const QMap<QString, StageMotorInfo> &motors)
{
    motorDefinitions_ = motors;
    ui->motorComboBox->addItems(motors.keys());
    if(motorDefinitions_.contains("Default")){
        updateUiForMotor(motorDefinitions_["Default"]);
    }
}

void AxisControlWidget::setPosition(double physical_position)
{
    ui->currentPositionLabel->setText(QString::number(physical_position, 'f', displayPrecision_));
}

void AxisControlWidget::applyPreset(const AxisPreset &preset)
{
    ui->motorComboBox->setCurrentText(preset.motorName);
    if(preset.isAbsolute) {
        ui->absoluteRadioButton->setChecked(true);
    } else {
        ui->relativeRadioButton->setChecked(true);
    }
    ui->valueLineEdit->setText(QString::number(preset.value));
    ui->speedComboBox->setCurrentText(QString::number(preset.speed));

    // Manually trigger UI update for the new motor
    if (motorDefinitions_.contains(preset.motorName)) {
        updateUiForMotor(motorDefinitions_[preset.motorName]);
    }
}

void AxisControlWidget::updateUiForMotor(const StageMotorInfo &motor)
{
    displayPrecision_ = motor.display_precision;
    const QString& unit = motor.unit_symbol;
    ui->positionUnitLabel->setText(QString("Pos (%1):").arg(unit));
    ui->valueLineEdit->setPlaceholderText(unit);
    ui->rangeLabel->setText(QString("(0~%1 %2)").arg(motor.travel_range * 2).arg(unit));
}

void AxisControlWidget::on_removeButton_clicked() { emit removalRequested(currentAxisNumber_); }
void AxisControlWidget::on_cwButton_clicked() { emit moveRequested(currentAxisNumber_, false); }
void AxisControlWidget::on_ccwButton_clicked() { emit moveRequested(currentAxisNumber_, true); }
void AxisControlWidget::on_originButton_clicked() { emit originRequested(currentAxisNumber_); }
void AxisControlWidget::on_importButton_clicked() { emit importRequested(currentAxisNumber_); }

void AxisControlWidget::on_motorComboBox_currentIndexChanged(int index)
{
    QString motorName = ui->motorComboBox->itemText(index);
    if (motorDefinitions_.contains(motorName)) {
        updateUiForMotor(motorDefinitions_[motorName]);
        emit motorSelectionChanged(currentAxisNumber_, motorName);
    }
}

