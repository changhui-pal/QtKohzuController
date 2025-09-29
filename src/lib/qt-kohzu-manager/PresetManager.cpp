#include "PresetManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

PresetManager::PresetManager(QObject *parent) : QObject(parent) {}

QString PresetManager::getPresetsDirectory() {
    QString path = "";
    path += "/resources/presets";
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return path;
}

QString PresetManager::getPresetFilePath(int axisNumber) {
    return getPresetsDirectory() + QString("/axis_%1.json").arg(axisNumber);
}

QList<AxisPreset> PresetManager::loadPresets(int axisNumber) {
    QList<AxisPreset> presets;
    QFile file(getPresetFilePath(axisNumber));
    if (!file.open(QIODevice::ReadOnly)) {
        return presets;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray array = doc.array();

    for (const QJsonValue &value : array) {
        QJsonObject obj = value.toObject();
        AxisPreset preset;
        preset.id = QUuid(obj["id"].toString());
        preset.motorName = obj["motorName"].toString();
        preset.isAbsolute = obj["isAbsolute"].toBool();
        preset.value = obj["value"].toDouble();
        preset.speed = obj["speed"].toInt();
        presets.append(preset);
    }
    return presets;
}

void PresetManager::savePresets(int axisNumber, const QList<AxisPreset>& presets) {
    QJsonArray array;
    for (const AxisPreset &preset : presets) {
        QJsonObject obj;
        obj["id"] = preset.id.toString();
        obj["motorName"] = preset.motorName;
        obj["isAbsolute"] = preset.isAbsolute;
        obj["value"] = preset.value;
        obj["speed"] = preset.speed;
        array.append(obj);
    }

    QFile file(getPresetFilePath(axisNumber));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(array).toJson());
    }
}

void PresetManager::addPreset(int axisNumber, const AxisPreset &newPreset) {
    QList<AxisPreset> presets = loadPresets(axisNumber);
    presets.prepend(newPreset); // Add to the top of the list
    savePresets(axisNumber, presets);
}
