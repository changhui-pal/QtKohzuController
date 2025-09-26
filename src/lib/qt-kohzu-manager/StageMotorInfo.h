#ifndef STAGEMOTORINFO_H
#define STAGEMOTORINFO_H

#include <QString>
#include <QMap>

// 모터의 단위를 구분하기 위한 열거형
enum class UnitType {
    Linear,  // 선형 이동 (mm)
    Angular  // 각도 이동 (°)
};

// 각 스테이지 모터의 물리적 특성을 정의하는 구조체
struct StageMotorInfo {
    QString name;
    UnitType unit_type;
    QString unit_symbol;      // UI에 표시될 단위 심볼 ("mm" or "°")
    double value_per_pulse;   // 1 half-step 펄스 당 이동하는 값 (mm 또는 °)
    double travel_range;      // 이동 가능한 최대 범위 (± 값)
    int display_precision;    // UI에 표시할 소수점 자릿수
};

// 애플리케이션 전체에서 사용할 모터 모델 목록을 정의하고 제공하는 함수
// 실제 Kohzu 장비의 카탈로그 스펙을 기반으로 작성되었습니다.
inline QMap<QString, StageMotorInfo> getMotorDefinitions() {
    QMap<QString, StageMotorInfo> definitions;

    definitions["Default"] = {"Default", UnitType::Linear, "pulse", 1.0, 1000000.0, 0};

    // Rotation Stage (각도)
    definitions["RA04A-W"] = {"RA04A-W", UnitType::Angular, "°", 0.002, 177.0, 4};

    // Z-axis Linear Stage (선형)
    definitions["ZA05A-W1"] = {"ZA05A-W1", UnitType::Linear, "mm", 0.00025, 3.3, 5}; // 0.25µm/pulse

    // Swing Arc Stage (각도)
    definitions["SA05A-R2B"] = {"SA05A-R2B", UnitType::Angular, "°", 0.000637, 3.5, 6};

    // X-axis Linear Stage (선형)
    definitions["XA05A-R201"] = {"XA05A-R201", UnitType::Linear, "mm", 0.0005, 7.5, 4}; // 0.5µm/pulse

    // Z-axis Linear Stage (선형)
    definitions["ZA10A-32F01"] = {"ZA10A-32F01", UnitType::Linear, "mm", 0.00005, 15.0, 5}; // 1.0µm/pulse

    return definitions;
}

#endif // STAGEMOTORINFO_H

