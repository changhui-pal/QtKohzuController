#include "AxisControlWidget.h"
#include "ui_AxisControlWidget.h"

AxisControlWidget::AxisControlWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AxisControlWidget)
{
    ui->setupUi(this);
    ui->absoluteRadioButton->setChecked(true);
}

AxisControlWidget::~AxisControlWidget()
{
    delete ui;
}

int AxisControlWidget::getAxisNumber() const
{
    return ui->axisSpinBox->value();
}

void AxisControlWidget::setPosition(int position)
{
    ui->currentPositionLabel->setText(QString::number(position));
}

void AxisControlWidget::setAxisNumber(int axisNumber)
{
    ui->axisSpinBox->setValue(axisNumber);
}

void AxisControlWidget::on_cwButton_clicked()
{
    int axis = ui->axisSpinBox->value();
    int value = ui->valueLineEdit->text().toInt();
    bool isAbsolute = ui->absoluteRadioButton->isChecked();
    emit moveRequested(axis, value, isAbsolute);
}

void AxisControlWidget::on_ccwButton_clicked()
{
    int axis = ui->axisSpinBox->value();
    int value = ui->valueLineEdit->text().toInt();
    bool isAbsolute = ui->absoluteRadioButton->isChecked();
    emit moveRequested(axis, -value, isAbsolute);
}

void AxisControlWidget::on_removeButton_clicked()
{
    emit removalRequested(ui->axisSpinBox->value());
}

void AxisControlWidget::on_axisSpinBox_valueChanged(int arg1)
{
    ui->container->setTitle(QString("Axis %1").arg(arg1));
}
