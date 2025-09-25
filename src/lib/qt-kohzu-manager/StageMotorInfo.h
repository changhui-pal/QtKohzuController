#ifndef STAGEMOTORINFO_H
#define STAGEMOTORINFO_H

#include <QString>
#include <QMap>

// 각 스테이지 모터의 물리적 특성을 정의하는 구조체
struct StageMotorInfo {
    QString name;
    double pulse_per_mm; // 1mm를 이동하는 데 필요한 펄스 수
    double travel_range_mm; // 이동 가능한 최대 범위 (± 값)
};

// 애플리케이션 전체에서 사용할 모터 모델 목록을 정의하고 제공하는 함수
inline QMap<QString, StageMotorInfo> getMotorDefinitions() {
    QMap<QString, StageMotorInfo> definitions;

    // ARIES 컨트롤러는 기본적으로 half step (분해능 400)을 사용합니다.
    // 예시: 2mm pitch 볼 스크류, 1.8도 스텝 모터 -> 1회전(360도) = 200 full steps = 400 half steps
    // 1회전 당 2mm 이동하므로, 400 pulses / 2mm = 200 pulses/mm
    definitions["Default"] = {"Default", 4000.0, 3.0}; // 펄스 단위를 그대로 사용할 기본 옵션
    definitions["KTM0650"] = {"KTM0650", 4000.0, 3.0};   // 예시: 4000 pulses/mm, ±3mm
    definitions["SAM-40"] = {"SAM-40", 2000.0, 3.0};      // 예시: 2000 pulses/mm, ±3mm

    return definitions;
}

#endif // STAGEMOTORINFO_H
