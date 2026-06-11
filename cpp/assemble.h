#pragma once
#include <string>

// ─── Phase 1: 전역 선택 상태 (Phase 2에서 CarSelection 구조체로 교체 예정) ──
extern int g_stack[5];

enum QuestionType {
    CarType_Q = 0,
    Engine_Q,
    BrakeSystem_Q,
    SteeringSystem_Q,
    Run_Test
};

// Phase 1: C enum 유지 (Phase 2에서 enum class로 교체 예정)
enum CarTypeVal    { SEDAN = 1, SUV = 2, TRUCK = 3 };
enum EngineVal     { GM = 1, TOYOTA = 2, WIA = 3, BROKEN_ENGINE = 4 };
enum BrakeVal      { MANDO = 1, CONTINENTAL = 2, BOSCH_B = 3 };
enum SteeringVal   { BOSCH_S = 1, MOBIS = 2 };

// Phase 1-5: DRY — isValidCheck() + testProducedCar()를 단일 구조체로 통합
struct ValidationResult {
    bool ok;            // Phase 1-2: int → bool
    std::string reason;
};

// ─── 함수 선언 ───────────────────────────────────────────────────────────────
void delay(int ms);
bool isValidRange(int step, int ans);

void selectCarType(int answer);
void selectEngine(int answer);
void selectBrakeSystem(int answer);       // Phase 1-2: selectbrakeSystem → selectBrakeSystem
void selectSteeringSystem(int answer);

ValidationResult validate();              // Phase 1-5: 통합 검증 (ok + reason)
void runProducedCar();

void showCarTypeMenu();
void showEngineMenu();
void showBrakeMenu();
void showSteeringMenu();
void showRunTestMenu();
