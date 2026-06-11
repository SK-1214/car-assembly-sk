# Refactoring Plan — assemble.cpp (차량 조립 시스템)

## 배경

**대상 파일:** `cpp/assemble.cpp`  
**문제 요약 (Day2.pdf p.24):**
- 절차지향식 코드로 유지보수 어려움
- 안전하지 않은 문법 사용 (C-style 함수, 전역 배열, busy-wait 등)
- 확장성 미고려 (새 차량 타입·부품 추가 시 기존 코드 전체 수정 필요)
- 유닛테스트 없음 (Google Mock 1.11.0이 프로젝트에 포함되어 있으나 미사용)

**핵심 설계 원칙:** OCP (Open/Closed Principle)  
> 새로운 차량 타입·부품·규칙을 추가할 때 기존 코드를 수정하지 않고 새 클래스를 추가하는 것만으로 확장 가능한 구조

**진행 방향:** 작은 변경(method level) → 큰 변경(class level) 순서로 단계별 진행

---

## Phase 1 — Method Level (함수 단위 수정)

### 1-1. 버그 수정

| 위치 | 문제 | 수정 |
|------|------|------|
| `selectSteeringSystem()` | "BOSCH **제동장치**를 선택", "MOBIS **제동장치**를 선택" → 조향장치로 출력해야 함 | 문자열 "제동장치" → "조향장치" |
| `runProducedCar()` | `"Brake System : Mando"` 끝에 `\n` 누락 | `\n` 추가 |
| `isValidCheck()` | PDF 명세에 없는 역방향 조건(`brakeSystem != BOSCH_B && steeringSystem == BOSCH_S`)이 코드에 존재 | 해당 else-if 블록 제거 |

### 1-2. 함수 시그니처 / 반환 타입 정리

- `isValidCheck()`: 반환 타입 `int` → `bool`
- `isValidCheck()` 마지막 unreachable `return true;` 제거
- `selectbrakeSystem()`: → `selectBrakeSystem()` (camelCase 통일)

### 1-3. unsafe C-style 문법 → 안전한 C++ 문법 교체

| 현재 (C-style) | 교체 (C++) |
|----------------|-----------|
| `#include <stdio.h>`, `<string.h>`, `<stdlib.h>` | `#include <iostream>`, `<string>` |
| `printf(...)` | `std::cout << ...` |
| `fgets(buf, sizeof(buf), stdin)` | `std::getline(std::cin, line)` |
| `strtok_s(buf, "\r", &ctx)` (Windows 전용) | `std::string::erase` + `find` |
| `strtol(buf, &check, 10)` | `std::stoi()` + `std::invalid_argument` 예외 처리 |
| `char buf[100]` 전역 | 지역 `std::string` |


### 1-4. 선택 함수 내 독립 if 체인 → if-else if 정리

`selectCarType()`, `selectEngine()`, `selectBrakeSystem()`, `selectSteeringSystem()` 모두
독립 `if`를 나열 → `if-else if` 또는 배열 조회로 단일 출력 경로 확보

### 1-5. `testProducedCar()` — `isValidCheck()` 중복 제거 (DRY)

현재 `testProducedCar()`는 `isValidCheck()`의 조건을 그대로 복사해 메시지만 다르게 출력.  
→ 실패 원인 메시지를 함께 반환하는 단일 함수로 통합

---

## Phase 2 — 데이터 구조 레벨 (중간 단계)

> Phase 3의 인터페이스 설계로 넘어가기 전, 전역 상태와 타입 안전성을 먼저 정리한다.

### 2-1. 전역 배열 제거 → 명명된 구조체

```cpp
// Before: 전역 int stack[10] — index 의미 불분명, 크기 여유분 불명확
int stack[10];

// After: 선택 상태를 구조체로 묶기 (Phase 3에서 interface 타입으로 교체됨)
struct CarSelection {
    int carType;
    int engine;
    int brakeSystem;
    int steeringSystem;
};
```

### 2-2. C enum → `enum class` (타입 안전성 — Phase 3 인터페이스 전환 전 임시 단계)

```cpp
// Before: C-style enum — 다른 enum 값과 암묵 비교 가능, int 자동 변환
enum CarType { SEDAN = 1, SUV, TRUCK };

// After: scoped enum — 타입 혼용 컴파일 에러로 잡힘
enum class CarType     { SEDAN = 1, SUV, TRUCK };
enum class Engine      { GM = 1, TOYOTA, WIA, BROKEN };
enum class BrakeSystem { MANDO = 1, CONTINENTAL, BOSCH };
enum class SteeringSystem { BOSCH = 1, MOBIS };
```

> **Note:** 이 `enum class`들은 Phase 3에서 각각 인터페이스 계층으로 대체되어 제거된다.

### 2-3. `main()` 분해

현재 `main()`은 UI 루프 + 입력 파싱 + 상태 전이 + 비즈니스 로직이 혼재.  
→ 세 개의 함수로 분리:

- `int readInput()` — 입력 읽기 + 숫자 파싱
- `bool handleStep(int step, int answer, CarSelection&)` — 상태 전이
- `main()` — 루프 제어만 담당

---

## Phase 3 — Class Level / OCP 기반 인터페이스 설계

> Phase 2의 `enum class`를 제거하고, 4개의 도메인 각각을 인터페이스로 추상화한다.  
> **확장 = 새 클래스 추가만으로 완결. 기존 코드 수정 없음.**

### 3-1. `ICarType` 인터페이스

```cpp
// 인터페이스
class ICarType {
public:
    virtual ~ICarType() = default;
    virtual std::string getName() const = 0;
};

// 구체 구현
class Sedan : public ICarType {
public:
    std::string getName() const override { return "Sedan"; }
};
class SUV   : public ICarType { ... };
class Truck : public ICarType { ... };

// OCP 적용: 새 차량 타입 추가 시 → Van : public ICarType { } 만 추가
//           Sedan, SUV, Truck 코드 및 Validator 코드 수정 없음
```

### 3-2. `IEngine` 인터페이스

```cpp
class IEngine {
public:
    virtual ~IEngine() = default;
    virtual std::string getName() const = 0;
    virtual bool isOperational() const = 0;  // 고장 엔진 분기 제거
};

class GmEngine     : public IEngine { std::string getName() const override { return "GM"; }     bool isOperational() const override { return true; } };
class ToyotaEngine : public IEngine { ... };
class WiaEngine    : public IEngine { ... };
class BrokenEngine : public IEngine {
    std::string getName() const override { return "고장난 엔진"; }
    bool isOperational() const override { return false; }
    // runProducedCar()의 if (stack[Engine_Q] == 4) 분기가 이 클래스로 흡수됨
};

// OCP 적용: 새 엔진 제조사 추가 → HyundaiEngine : public IEngine { } 만 추가
```

### 3-3. `IBrakeSystem` 인터페이스

```cpp
class IBrakeSystem {
public:
    virtual ~IBrakeSystem() = default;
    virtual std::string getName() const = 0;
};

class MandoBrakeSystem       : public IBrakeSystem { ... };
class ContinentalBrakeSystem : public IBrakeSystem { ... };
class BoschBrakeSystem       : public IBrakeSystem { ... };

// OCP 적용: 새 제동장치 추가 → AktiebolaegetBrakeSystem : public IBrakeSystem { } 만 추가
```

### 3-4. `ISteeringSystem` 인터페이스

```cpp
class ISteeringSystem {
public:
    virtual ~ISteeringSystem() = default;
    virtual std::string getName() const = 0;
};

class BoschSteeringSystem : public ISteeringSystem { ... };
class MobisSteeringSystem : public ISteeringSystem { ... };

// OCP 적용: 새 조향장치 추가 → ZFSteeringSystem : public ISteeringSystem { } 만 추가
```

### 3-5. `Car` — 4개 인터페이스를 조합하는 도메인 객체

```cpp
class Car {
public:
    std::unique_ptr<ICarType>       carType;
    std::unique_ptr<IEngine>        engine;
    std::unique_ptr<IBrakeSystem>   brakeSystem;
    std::unique_ptr<ISteeringSystem> steeringSystem;

    bool isRunnable() const { return engine && engine->isOperational(); }
    std::string describe() const;
};
```

`enum class` 기반 분기(`if carType == SEDAN ...`)가 전부 사라지고,
다형성(virtual dispatch)이 분기를 대체한다.

### 3-6. `ICompatibilityRule` + `Validator` — 호환성 규칙도 OCP 적용

호환성 규칙을 인터페이스화하면 새 규칙 추가 시 `Validator` 코드를 수정하지 않아도 된다.

```cpp
// 규칙 인터페이스
class ICompatibilityRule {
public:
    virtual ~ICompatibilityRule() = default;
    virtual bool isSatisfied(const Car& car) const = 0;
    virtual std::string failureMessage() const = 0;
};

// 구체 규칙 (각 규칙이 독립 클래스)
class BoschBrakeRequiresBoschSteering : public ICompatibilityRule {
    bool isSatisfied(const Car& car) const override {
        // BoschBrakeSystem 여부를 dynamic_cast 또는 타입 태그로 확인
    }
    std::string failureMessage() const override {
        return "Bosch 제동장치에는 Bosch 조향장치만 사용 가능";
    }
};
class ContinentalNotForSedan : public ICompatibilityRule { ... };
class ToyotaNotForSuv        : public ICompatibilityRule { ... };
class WiaNotForTruck         : public ICompatibilityRule { ... };
class MandoNotForTruck       : public ICompatibilityRule { ... };

// Validator — 규칙 목록을 순회할 뿐, 규칙 내용을 알지 못함
class Validator {
    std::vector<std::unique_ptr<ICompatibilityRule>> rules;
public:
    Validator();  // 기본 규칙 5개 등록
    void addRule(std::unique_ptr<ICompatibilityRule> rule);  // 새 규칙 추가 (수정 없이 확장)

    struct Result { bool ok; std::string reason; };
    Result validate(const Car& car) const;
};

// OCP 적용: 새 호환성 규칙 추가 → NewRule : public ICompatibilityRule { } + addRule() 호출만 추가
//           Validator 내부 로직 수정 없음
```

### 3-7. `ConsoleUI` — 출력 로직 분리

메뉴 출력 함수들을 별도 클래스로 분리해 비즈니스 로직과 결합 해제.  
각 `ICarType`, `IEngine`, `IBrakeSystem`, `ISteeringSystem` 구현 목록을 받아 동적으로 메뉴를 생성하면,
새 부품 추가 시 메뉴 코드도 수정이 필요 없다.

```cpp
class ConsoleUI {
public:
    void showCarTypeMenu(const std::vector<ICarType*>& types) const;
    void showEngineMenu(const std::vector<IEngine*>& engines) const;
    void showBrakeMenu(const std::vector<IBrakeSystem*>& brakes) const;
    void showSteeringMenu(const std::vector<ISteeringSystem*>& steerings) const;
    void showResult(const Validator::Result& result) const;
};
```

### 3-8. `CarAssembler` — 조립 진행 제어

선택 흐름을 제어하고 `Car` 객체를 완성하는 역할.  
UI, Validator, Car 사이의 결합을 최소화한다.

```cpp
class CarAssembler {
    ConsoleUI ui;
    Validator validator;
public:
    Car assemble();   // 단계별 선택 루프 → 완성된 Car 반환
    void run(const Car& car) const;
    void test(const Car& car) const;
};
```

---

## Phase 4 — 유닛 테스트 (Google Test / Mock 활용)

프로젝트에 `gmock.1.11.0` 패키지가 이미 포함되어 있으나 테스트 코드 없음.  
테스트 파일은 `tests/` 디렉터리에 기능 단위로 분리한다. (→ 파일 구조 참고)

---

### TC-1. `IEngine` 구현체 — `tests/test_engine.cpp`

```cpp
#include <gtest/gtest.h>
#include "engine/GmEngine.h"
#include "engine/ToyotaEngine.h"
#include "engine/WiaEngine.h"
#include "engine/BrokenEngine.h"

TEST(EngineTest, GmEngine_IsOperational) {
    GmEngine engine;
    EXPECT_TRUE(engine.isOperational());
    EXPECT_EQ(engine.getName(), "GM");
}

TEST(EngineTest, ToyotaEngine_IsOperational) {
    ToyotaEngine engine;
    EXPECT_TRUE(engine.isOperational());
    EXPECT_EQ(engine.getName(), "TOYOTA");
}

TEST(EngineTest, WiaEngine_IsOperational) {
    WiaEngine engine;
    EXPECT_TRUE(engine.isOperational());
    EXPECT_EQ(engine.getName(), "WIA");
}

TEST(EngineTest, BrokenEngine_IsNotOperational) {
    BrokenEngine engine;
    EXPECT_FALSE(engine.isOperational());
}
```

---

### TC-2. `Car::isRunnable()` — `tests/test_car.cpp`

```cpp
#include <gtest/gtest.h>
#include "Car.h"
#include "car_type/Sedan.h"
#include "engine/GmEngine.h"
#include "engine/BrokenEngine.h"
#include "brake_system/MandoBrakeSystem.h"
#include "steering_system/MobisSteeringSystem.h"

TEST(CarTest, IsRunnable_WithNormalEngine) {
    Car car;
    car.carType       = std::make_unique<Sedan>();
    car.engine        = std::make_unique<GmEngine>();
    car.brakeSystem   = std::make_unique<MandoBrakeSystem>();
    car.steeringSystem = std::make_unique<MobisSteeringSystem>();
    EXPECT_TRUE(car.isRunnable());
}

TEST(CarTest, IsNotRunnable_WithBrokenEngine) {
    Car car;
    car.carType       = std::make_unique<Sedan>();
    car.engine        = std::make_unique<BrokenEngine>();
    car.brakeSystem   = std::make_unique<MandoBrakeSystem>();
    car.steeringSystem = std::make_unique<MobisSteeringSystem>();
    EXPECT_FALSE(car.isRunnable());
}
```

---

### TC-3. `Validator` 호환성 규칙 — `tests/test_validator.cpp`

```cpp
#include <gtest/gtest.h>
#include "Validator.h"
#include "Car.h"
#include "car_type/Sedan.h"
#include "car_type/SUV.h"
#include "car_type/Truck.h"
#include "engine/GmEngine.h"
#include "engine/ToyotaEngine.h"
#include "engine/WiaEngine.h"
#include "brake_system/MandoBrakeSystem.h"
#include "brake_system/ContinentalBrakeSystem.h"
#include "brake_system/BoschBrakeSystem.h"
#include "steering_system/BoschSteeringSystem.h"
#include "steering_system/MobisSteeringSystem.h"

class ValidatorTest : public ::testing::Test {
protected:
    Validator validator;

    Car makeCar(ICarType* ct, IEngine* eng, IBrakeSystem* br, ISteeringSystem* st) {
        Car car;
        car.carType        = std::unique_ptr<ICarType>(ct);
        car.engine         = std::unique_ptr<IEngine>(eng);
        car.brakeSystem    = std::unique_ptr<IBrakeSystem>(br);
        car.steeringSystem = std::unique_ptr<ISteeringSystem>(st);
        return car;
    }
};

// 제한조건 1: Bosch 제동장치 → Bosch 조향장치 강제
TEST_F(ValidatorTest, BoschBrake_RequiresBoschSteering_Fail) {
    auto car = makeCar(new Sedan(), new GmEngine(),
                       new BoschBrakeSystem(), new MobisSteeringSystem());
    auto result = validator.validate(car);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.reason, "Bosch 제동장치에는 Bosch 조향장치만 사용 가능");
}

TEST_F(ValidatorTest, BoschBrake_WithBoschSteering_Pass) {
    auto car = makeCar(new Sedan(), new GmEngine(),
                       new BoschBrakeSystem(), new BoschSteeringSystem());
    EXPECT_TRUE(validator.validate(car).ok);
}

// 제한조건 2-1: Continental — Sedan 불가
TEST_F(ValidatorTest, Continental_OnSedan_Fail) {
    auto car = makeCar(new Sedan(), new GmEngine(),
                       new ContinentalBrakeSystem(), new MobisSteeringSystem());
    auto result = validator.validate(car);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.reason, "Sedan에는 Continental 제동장치 사용 불가");
}

// 제한조건 2-2: Toyota — SUV 불가
TEST_F(ValidatorTest, Toyota_OnSuv_Fail) {
    auto car = makeCar(new SUV(), new ToyotaEngine(),
                       new MandoBrakeSystem(), new MobisSteeringSystem());
    auto result = validator.validate(car);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.reason, "SUV에는 TOYOTA 엔진 사용 불가");
}

// 제한조건 2-3: WIA — Truck 불가
TEST_F(ValidatorTest, Wia_OnTruck_Fail) {
    auto car = makeCar(new Truck(), new WiaEngine(),
                       new MandoBrakeSystem(), new MobisSteeringSystem());
    auto result = validator.validate(car);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.reason, "Truck에는 WIA 엔진 사용 불가");
}

// 제한조건 2-4: Mando — Truck 불가
TEST_F(ValidatorTest, Mando_OnTruck_Fail) {
    auto car = makeCar(new Truck(), new GmEngine(),
                       new MandoBrakeSystem(), new MobisSteeringSystem());
    auto result = validator.validate(car);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.reason, "Truck에는 Mando 제동장치 사용 불가");
}

// 유효 조합 — PASS
TEST_F(ValidatorTest, ValidCombination_Sedan_Pass) {
    auto car = makeCar(new Sedan(), new GmEngine(),
                       new MandoBrakeSystem(), new MobisSteeringSystem());
    EXPECT_TRUE(validator.validate(car).ok);
}

TEST_F(ValidatorTest, ValidCombination_Truck_Pass) {
    auto car = makeCar(new Truck(), new GmEngine(),
                       new BoschBrakeSystem(), new BoschSteeringSystem());
    EXPECT_TRUE(validator.validate(car).ok);
}

// OCP 확장성: 새 규칙을 addRule()로 추가해도 기존 테스트 영향 없음
TEST_F(ValidatorTest, AddRule_NewRuleApplied) {
    class AlwaysFailRule : public ICompatibilityRule {
    public:
        bool isSatisfied(const Car&) const override { return false; }
        std::string failureMessage() const override { return "항상 실패"; }
    };
    validator.addRule(std::make_unique<AlwaysFailRule>());
    auto car = makeCar(new Sedan(), new GmEngine(),
                       new MandoBrakeSystem(), new MobisSteeringSystem());
    EXPECT_FALSE(validator.validate(car).ok);
}
```

---

## 파일 구조 (기능 단위 분리)

현재 모든 코드가 `assemble.cpp` 한 파일에 집중되어 있다.  
Phase 3 완료 후 아래 구조로 분리한다.

```
cpp/
├── interfaces/                     # 순수 추상 인터페이스 (의존성 방향의 최상위)
│   ├── ICarType.h
│   ├── IEngine.h
│   ├── IBrakeSystem.h
│   ├── ISteeringSystem.h
│   └── ICompatibilityRule.h
│
├── car_type/                       # ICarType 구현체
│   ├── Sedan.h
│   ├── SUV.h
│   └── Truck.h
│
├── engine/                         # IEngine 구현체
│   ├── GmEngine.h
│   ├── ToyotaEngine.h
│   ├── WiaEngine.h
│   └── BrokenEngine.h
│
├── brake_system/                   # IBrakeSystem 구현체
│   ├── MandoBrakeSystem.h
│   ├── ContinentalBrakeSystem.h
│   └── BoschBrakeSystem.h
│
├── steering_system/                # ISteeringSystem 구현체
│   ├── BoschSteeringSystem.h
│   └── MobisSteeringSystem.h
│
├── rules/                          # ICompatibilityRule 구현체 (규칙 단위 분리)
│   ├── BoschBrakeRequiresBoschSteering.h
│   ├── ContinentalNotForSedan.h
│   ├── ToyotaNotForSuv.h
│   ├── WiaNotForTruck.h
│   └── MandoNotForTruck.h
│
├── Car.h                           # Car 도메인 객체
├── Validator.h / Validator.cpp     # 규칙 목록 관리 + validate()
├── ConsoleUI.h / ConsoleUI.cpp     # 콘솔 입출력 전담
├── CarAssembler.h / CarAssembler.cpp  # 조립 흐름 제어
└── main.cpp                        # 진입점 (CarAssembler 생성·실행만)
```

```
tests/                              # 기능 단위 TC 파일 분리
├── test_engine.cpp                 # TC-1: IEngine 구현체 테스트
├── test_car.cpp                    # TC-2: Car::isRunnable() 테스트
├── test_validator.cpp              # TC-3: Validator 호환성 규칙 전체 테스트
├── test_car_type.cpp               # ICarType 구현체 getName() 테스트
├── test_brake_system.cpp           # IBrakeSystem 구현체 getName() 테스트
└── test_steering_system.cpp        # ISteeringSystem 구현체 getName() 테스트
```

### 파일 분리 원칙

| 원칙 | 적용 내용 |
|------|----------|
| 인터페이스와 구현 분리 | `interfaces/` 는 구현체 헤더를 include 하지 않음 |
| 기능 단위 디렉터리 | 같은 인터페이스의 구현체끼리 한 폴더로 묶음 |
| 규칙 단위 TC 파일 | 테스트 파일이 하나의 클래스(또는 기능)만 책임짐 |
| Header-only 구현체 | 단순한 구현체(`getName()` 등)는 `.h`만으로 구성, 복잡한 클래스는 `.cpp` 분리 |

---

## 클래스 다이어그램 (최종 구조)

```
ICarType            IEngine              IBrakeSystem         ISteeringSystem
  ├─ Sedan            ├─ GmEngine          ├─ MandoBrakeSystem   ├─ BoschSteeringSystem
  ├─ SUV              ├─ ToyotaEngine       ├─ ContinentalBrake   └─ MobisSteeringSystem
  └─ Truck            ├─ WiaEngine          └─ BoschBrakeSystem
  (└─ Van ← 추가 예시) └─ BrokenEngine

Car
  ├─ unique_ptr<ICarType>
  ├─ unique_ptr<IEngine>
  ├─ unique_ptr<IBrakeSystem>
  └─ unique_ptr<ISteeringSystem>

ICompatibilityRule
  ├─ BoschBrakeRequiresBoschSteering
  ├─ ContinentalNotForSedan
  ├─ ToyotaNotForSuv
  ├─ WiaNotForTruck
  └─ MandoNotForTruck

Validator ──────────── vector<unique_ptr<ICompatibilityRule>>
CarAssembler ────────── ConsoleUI + Validator
```

---

## 변경 순서 요약

```
Phase 1 (Method — 함수 단위)
  1-1  버그 수정 (출력 문자열, \n, 잘못된 규칙)
  1-2  반환 타입·네이밍 정리 (bool, camelCase)
  1-3  C-style → C++ 문법 교체 (iostream, string, stoi)
  1-4  delay() busy-wait → std::this_thread::sleep_for
  1-5  독립 if → if-else if 정리
  1-6  testProducedCar() 중복 제거 (DRY)

Phase 2 (Data structure — 중간 단계)
  2-1  전역 stack[] → CarSelection 구조체
  2-2  C enum → enum class (타입 안전성 확보, Phase 3 전 임시)
  2-3  main() 분해 (readInput / handleStep / main)

Phase 3 (Class / OCP — 인터페이스 계층)
  3-1  ICarType 인터페이스 + Sedan / SUV / Truck 구현
  3-2  IEngine 인터페이스 + Gm / Toyota / Wia / BrokenEngine 구현
  3-3  IBrakeSystem 인터페이스 + Mando / Continental / Bosch 구현
  3-4  ISteeringSystem 인터페이스 + Bosch / Mobis 구현
  3-5  Car 클래스 (4개 인터페이스 포인터 조합)
  3-6  ICompatibilityRule 인터페이스 + 규칙별 구현 + Validator
  3-7  ConsoleUI 클래스 (동적 메뉴 생성)
  3-8  CarAssembler 클래스 (조립 흐름 제어)
  ※ Phase 2의 enum class 전부 제거

Phase 4 (Test)
  4-1  ICompatibilityRule 구현체별 유닛 테스트
  4-2  IEngine::isOperational() / Car::isRunnable() 유닛 테스트
  4-3  Validator::addRule() 확장성 테스트
```

---

## OCP 적용 효과 — 확장 시나리오

| 변경 요구 | 수정 파일 | 기존 코드 변경 |
|----------|----------|--------------|
| 새 차량 타입 `Van` 추가 | `Van.h` 신규 추가 | 없음 |
| 새 엔진 `HyundaiEngine` 추가 | `HyundaiEngine.h` 신규 추가 | 없음 |
| 새 호환성 규칙 추가 | `NewRule.h` 신규 추가 + `main()`에서 `addRule()` 호출 | 없음 |
| 새 메뉴 항목 (부품 추가 시) | 없음 (ConsoleUI가 목록을 동적으로 순회) | 없음 |
