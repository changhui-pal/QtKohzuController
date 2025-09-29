#ifndef PRESETMANAGER_H
#define PRESETMANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QUuid>

// 단일 프리셋 데이터를 담는 구조체
struct AxisPreset {
    QUuid id;
    QString motorName;
    bool isAbsolute;
    double value;
    int speed;

    bool operator==(const AxisPreset& other) const { return id == other.id; }
};

class PresetManager : public QObject
{
    Q_OBJECT
public:
    explicit PresetManager(QObject *parent = nullptr);

    QList<AxisPreset> loadPresets(int axisNumber);
    void savePresets(int axisNumber, const QList<AxisPreset>& presets);
    void addPreset(int axisNumber, const AxisPreset& preset);

private:
    QString getPresetsDirectory();
    QString getPresetFilePath(int axisNumber);
};

#endif // PRESETMANAGER_H
