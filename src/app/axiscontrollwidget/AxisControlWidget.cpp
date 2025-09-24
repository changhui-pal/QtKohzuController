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
    return currentAxisNumber_;
}

void AxisControlWidget::setAxisNumber(int axisNumber)
{
    currentAxisNumber_ = axisNumber;
    ui->axisNumberLabel->setText(QString::number(axisNumber));
    ui->container->setTitle(QString("Axis %1").arg(axisNumber));
}

void AxisControlWidget::setPosition(int position)
{
    ui->currentPositionLabel->setText(QString::number(position));
}

void AxisControlWidget::on_cwButton_clicked()
{
    int value = ui->valueLineEdit->text().toInt();
    bool isAbsolute = ui->absoluteRadioButton->isChecked();
    emit moveRequested(currentAxisNumber_, value, isAbsolute);
}

void AxisControlWidget::on_ccwButton_clicked()
{
    int value = ui->valueLineEdit->text().toInt();
    bool isAbsolute = ui->absoluteRadioButton->isChecked();
    emit moveRequested(currentAxisNumber_, -value, isAbsolute);
}

void AxisControlWidget::on_removeButton_clicked()
{
    emit removalRequested(currentAxisNumber_);
}
