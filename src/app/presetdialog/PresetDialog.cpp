#include "PresetDialog.h"
#include "ui_PresetDialog.h"
#include <QListWidgetItem>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

PresetDialog::PresetDialog(int axis, PresetManager* manager, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PresetDialog),
    axisNumber_(axis),
    presetManager_(manager)
{
    ui->setupUi(this);
    setWindowTitle(QString("Axis %1 Presets").arg(axisNumber_));
    populateList();
}

PresetDialog::~PresetDialog()
{
    delete ui;
}

void PresetDialog::populateList()
{
    ui->listWidget->clear();
    currentPresets_ = presetManager_->loadPresets(axisNumber_);

    for (int i = 0; i < currentPresets_.size(); ++i) {
        const auto& preset = currentPresets_[i];

        QWidget* widget = new QWidget;
        QHBoxLayout* layout = new QHBoxLayout(widget);
        layout->setContentsMargins(5, 5, 5, 5);

        QString labelText = QString("Motor: %1, Mode: %2, Value: %3, Speed: %4")
                                .arg(preset.motorName)
                                .arg(preset.isAbsolute ? "Abs" : "Rel")
                                .arg(preset.value)
                                .arg(preset.speed);

        QLabel* label = new QLabel(labelText, widget);
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        QPushButton* applyButton = new QPushButton("Apply", widget);
        QPushButton* deleteButton = new QPushButton("Delete", widget);

        applyButton->setProperty("presetIndex", i);
        deleteButton->setProperty("presetIndex", i);

        connect(applyButton, &QPushButton::clicked, this, &PresetDialog::onApplyClicked);
        connect(deleteButton, &QPushButton::clicked, this, &PresetDialog::onDeleteClicked);

        layout->addWidget(label);
        layout->addWidget(applyButton);
        layout->addWidget(deleteButton);

        QListWidgetItem* item = new QListWidgetItem(ui->listWidget);
        item->setSizeHint(widget->sizeHint());
        ui->listWidget->addItem(item);
        ui->listWidget->setItemWidget(item, widget);
    }
}

void PresetDialog::onApplyClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        int index = button->property("presetIndex").toInt();
        if (index >= 0 && index < currentPresets_.size()) {
            emit presetApplied(currentPresets_[index]);
            accept();
        }
    }
}

void PresetDialog::onDeleteClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        int index = button->property("presetIndex").toInt();
        if (index >= 0 && index < currentPresets_.size()) {
            currentPresets_.removeAt(index);
            presetManager_->savePresets(axisNumber_, currentPresets_);
            populateList(); // Refresh the list
        }
    }
}
