#include "assemble.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>
#include <stdexcept>

int g_stack[5] = {0};

// Phase 1-3: busy-wait → std::this_thread::sleep_for
void delay(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// ─── 메뉴 출력 ────────────────────────────────────────────────────────────────
void showCarTypeMenu() {
    std::cout << "        ______________\n"
              << "       /|            |\n"
              << "  ____/_|_____________|____\n"
              << " |                      O  |\n"
              << " '-(@)----------------(@)--'\n"
              << "===============================\n"
              << "어떤 차량 타입을 선택할까요?\n"
              << "1. Sedan\n"
              << "2. SUV\n"
              << "3. Truck\n";
}

void showEngineMenu() {
    std::cout << "어떤 엔진을 탑재할까요?\n"
              << "0. 뒤로가기\n"
              << "1. GM\n"
              << "2. TOYOTA\n"
              << "3. WIA\n"
              << "4. 고장난 엔진\n";
}

void showBrakeMenu() {
    std::cout << "어떤 제동장치를 선택할까요?\n"
              << "0. 뒤로가기\n"
              << "1. MANDO\n"
              << "2. CONTINENTAL\n"
              << "3. BOSCH\n";
}

void showSteeringMenu() {
    std::cout << "어떤 조향장치를 선택할까요?\n"
              << "0. 뒤로가기\n"
              << "1. BOSCH\n"
              << "2. MOBIS\n";
}

void showRunTestMenu() {
    std::cout << "멋진 차량이 완성되었습니다.\n"
              << "어떤 동작을 할까요?\n"
              << "0. 처음 화면으로 돌아가기\n"
              << "1. RUN\n"
              << "2. Test\n";
}

// ─── 입력 범위 검사 (Phase 1-4: 독립 if → switch-case로 정리) ───────────────
bool isValidRange(int step, int ans) {
    switch (step) {
        case CarType_Q:
            if (ans < 1 || ans > 3) {
                std::cout << "ERROR :: 차량 타입은 1 ~ 3 범위만 선택 가능\n";
                return false;
            }
            break;
        case Engine_Q:
            if (ans < 0 || ans > 4) {
                std::cout << "ERROR :: 엔진은 1 ~ 4 범위만 선택 가능\n";
                return false;
            }
            break;
        case BrakeSystem_Q:
            if (ans < 0 || ans > 3) {
                std::cout << "ERROR :: 제동장치는 1 ~ 3 범위만 선택 가능\n";
                return false;
            }
            break;
        case SteeringSystem_Q:
            if (ans < 0 || ans > 2) {
                std::cout << "ERROR :: 조향장치는 1 ~ 2 범위만 선택 가능\n";
                return false;
            }
            break;
        case Run_Test:
            if (ans < 0 || ans > 2) {
                std::cout << "ERROR :: Run 또는 Test 중 하나를 선택 필요\n";
                return false;
            }
            break;
    }
    return true;
}

// ─── 부품 선택 (Phase 1-4: 독립 if → if-else if로 정리) ─────────────────────
void selectCarType(int answer) {
    g_stack[CarType_Q] = answer;
    if      (answer == SEDAN)  std::cout << "차량 타입으로 Sedan을 선택하셨습니다.\n";
    else if (answer == SUV)    std::cout << "차량 타입으로 SUV을 선택하셨습니다.\n";
    else if (answer == TRUCK)  std::cout << "차량 타입으로 Truck을 선택하셨습니다.\n";
}

void selectEngine(int answer) {
    g_stack[Engine_Q] = answer;
    if      (answer == GM)            std::cout << "GM 엔진을 선택하셨습니다.\n";
    else if (answer == TOYOTA)        std::cout << "TOYOTA 엔진을 선택하셨습니다.\n";
    else if (answer == WIA)           std::cout << "WIA 엔진을 선택하셨습니다.\n";
    else if (answer == BROKEN_ENGINE) std::cout << "고장난 엔진을 선택하셨습니다.\n";
}

// Phase 1-2: selectbrakeSystem → selectBrakeSystem (camelCase 통일)
void selectBrakeSystem(int answer) {
    g_stack[BrakeSystem_Q] = answer;
    if      (answer == MANDO)       std::cout << "MANDO 제동장치를 선택하셨습니다.\n";
    else if (answer == CONTINENTAL) std::cout << "CONTINENTAL 제동장치를 선택하셨습니다.\n";
    else if (answer == BOSCH_B)     std::cout << "BOSCH 제동장치를 선택하셨습니다.\n";
}

// Phase 1-1-a: "제동장치" → "조향장치" 오출력 버그 수정
void selectSteeringSystem(int answer) {
    g_stack[SteeringSystem_Q] = answer;
    if      (answer == BOSCH_S) std::cout << "BOSCH 조향장치를 선택하셨습니다.\n";
    else if (answer == MOBIS)   std::cout << "MOBIS 조향장치를 선택하셨습니다.\n";
}

// ─── Phase 1-5 DRY + Phase 1-1-c 버그 수정 + Phase 1-2 반환 타입 통합 ────────
// Before: isValidCheck() (int 반환) + testProducedCar() 로직 중복
// After:  validate() 한 곳에서 ok/reason 모두 반환 — 두 함수가 동일 소스 공유
// Phase 1-1-c: 명세에 없는 역방향 규칙 제거
//              (구: brakeSystem != BOSCH_B && steeringSystem == BOSCH_S → false)
ValidationResult validate() {
    if (g_stack[CarType_Q] == SEDAN && g_stack[BrakeSystem_Q] == CONTINENTAL)
        return {false, "Sedan에는 Continental 제동장치 사용 불가"};
    if (g_stack[CarType_Q] == SUV   && g_stack[Engine_Q] == TOYOTA)
        return {false, "SUV에는 TOYOTA 엔진 사용 불가"};
    if (g_stack[CarType_Q] == TRUCK && g_stack[Engine_Q] == WIA)
        return {false, "Truck에는 WIA 엔진 사용 불가"};
    if (g_stack[CarType_Q] == TRUCK && g_stack[BrakeSystem_Q] == MANDO)
        return {false, "Truck에는 Mando 제동장치 사용 불가"};
    if (g_stack[BrakeSystem_Q] == BOSCH_B && g_stack[SteeringSystem_Q] != BOSCH_S)
        return {false, "Bosch 제동장치에는 Bosch 조향장치만 사용 가능"};
    return {true, ""};
}

// Phase 1-1-b: Mando 출력 \n 누락 수정 (배열 기반으로 모든 항목 일관 처리)
void runProducedCar() {
    ValidationResult result = validate();
    if (!result.ok) {
        std::cout << "자동차가 동작되지 않습니다\n";
        return;
    }
    if (g_stack[Engine_Q] == BROKEN_ENGINE) {
        std::cout << "엔진이 고장나있습니다.\n"
                  << "자동차가 움직이지 않습니다.\n";
        return;
    }
    const std::string carNames[]   = {"", "Sedan", "SUV", "Truck"};
    const std::string engNames[]   = {"", "GM", "TOYOTA", "WIA"};
    const std::string brakeNames[] = {"", "Mando", "Continental", "Bosch"};
    const std::string steerNames[] = {"", "Bosch", "Mobis"};

    std::cout << "Car Type : " << carNames[g_stack[CarType_Q]]   << "\n"
              << "Engine   : " << engNames[g_stack[Engine_Q]]     << "\n"
              << "Brake    : " << brakeNames[g_stack[BrakeSystem_Q]] << "\n"
              << "Steering : " << steerNames[g_stack[SteeringSystem_Q]] << "\n"
              << "자동차가 동작됩니다.\n";
}

// ─── main (UNIT_TESTING 매크로 정의 시 제외) ─────────────────────────────────
#ifndef UNIT_TESTING
int main() {
    std::string line;
    int step = CarType_Q;

    while (true) {
        std::cout << "\033[H\033[2J";

        switch (step) {
            case CarType_Q:        showCarTypeMenu();   break;
            case Engine_Q:         showEngineMenu();    break;
            case BrakeSystem_Q:    showBrakeMenu();     break;
            case SteeringSystem_Q: showSteeringMenu();  break;
            case Run_Test:         showRunTestMenu();   break;
        }
        std::cout << "===============================\nINPUT > ";

        if (!std::getline(std::cin, line)) break;

        // Phase 1-3: strtok_s(Windows 전용) → C++ string 처리
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

        if (line == "exit") { std::cout << "바이바이\n"; break; }

        int answer;
        try {
            // Phase 1-3: strtol + checkNumber → std::stoi + 예외 처리
            answer = std::stoi(line);
        } catch (const std::invalid_argument&) {
            std::cout << "ERROR :: 숫자만 입력 가능\n";
            delay(800);
            continue;
        }

        if (!isValidRange(step, answer)) { delay(800); continue; }

        if (answer == 0) {
            step = (step == Run_Test) ? CarType_Q : step - 1;
            continue;
        }

        switch (step) {
            case CarType_Q:
                selectCarType(answer);     delay(800); step = Engine_Q;        break;
            case Engine_Q:
                selectEngine(answer);      delay(800); step = BrakeSystem_Q;   break;
            case BrakeSystem_Q:
                selectBrakeSystem(answer); delay(800); step = SteeringSystem_Q; break;
            case SteeringSystem_Q:
                selectSteeringSystem(answer); delay(800); step = Run_Test;     break;
            case Run_Test:
                if (answer == 1) {
                    runProducedCar();
                    delay(2000);
                } else if (answer == 2) {
                    std::cout << "Test...\n";
                    delay(1500);
                    ValidationResult r = validate();
                    if (r.ok) std::cout << "자동차 부품 조합 테스트 결과 : PASS\n";
                    else      std::cout << "자동차 부품 조합 테스트 결과 : FAIL\n" << r.reason << "\n";
                    delay(2000);
                }
                break;
        }
    }
    return 0;
}
#endif
