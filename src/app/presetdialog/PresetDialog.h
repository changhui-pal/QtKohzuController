#ifndef PRESETDIALOG_H
#define PRESETDIALOG_H

#include <QDialog>
#include <QList>
#include "PresetManager.h"

namespace Ui {
class PresetDialog;
}

class PresetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PresetDialog(int axis, PresetManager* manager, QWidget *parent = nullptr);
    ~PresetDialog();

signals:
    void presetApplied(const AxisPreset& preset);

private slots:
    void populateList();
    void onApplyClicked();
    void onDeleteClicked();

private:
    Ui::PresetDialog *ui;
    int axisNumber_;
    PresetManager* presetManager_;
    QList<AxisPreset> currentPresets_;
};

#endif // PRESETDIALOG_H
