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

// --- Getters ---
int AxisControlWidget::getAxisNumber() const { return currentAxisNumber_; }
QString AxisControlWidget::getSelectedMotorName() const { return ui->motorComboBox->currentText(); }
double AxisControlWidget::getInputValue() const { return ui->valueLineEdit->text().toDouble(); }
int AxisControlWidget::getSelectedSpeed() const { return ui->speedComboBox->currentText().toInt(); }
bool AxisControlWidget::isAbsoluteMode() const { return ui->absoluteRadioButton->isChecked(); }

// --- Setters / UI-Setup ---
void AxisControlWidget::setAxisNumber(int axisNumber)
{
    currentAxisNumber_ = axisNumber;
    ui->container->setTitle(QString("Axis %1").arg(axisNumber));
}

void AxisControlWidget::populateMotorDropdown(const QMap<QString, StageMotorInfo> &motors)
{
    for (const auto& name : motors.keys()) {
        ui->motorComboBox->addItem(name);
    }
}

void AxisControlWidget::setTravelRange(double range)
{
    ui->rangeLabel->setText(QString("(± %1 mm)").arg(range));
}

void AxisControlWidget::setPosition(double position_mm)
{
    ui->currentPositionLabel->setText(QString::number(position_mm, 'f', 4)); // 소수점 4자리까지 표시
}

// --- Slots ---
void AxisControlWidget::on_removeButton_clicked()
{
    emit removalRequested(currentAxisNumber_);
}

void AxisControlWidget::on_cwButton_clicked()
{
    emit moveRequested(currentAxisNumber_);
}

void AxisControlWidget::on_ccwButton_clicked()
{
    emit moveRequested(currentAxisNumber_);
}

void AxisControlWidget::on_motorComboBox_currentIndexChanged(const QString &motorName)
{
    emit motorSelectionChanged(currentAxisNumber_, motorName);
}

