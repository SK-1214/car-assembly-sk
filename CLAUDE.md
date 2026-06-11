# CLAUDE.md — 차량 조립 시스템 리팩터링 프로젝트

## 프로젝트 개요

절차지향으로 작성된 차량 조립 시뮬레이터(`cpp/assemble.cpp`)를 OCP 기반 객체지향 구조로 리팩터링한다.

- **언어:** C++20
- **빌드:** Visual Studio (sk_car.slnx), MSVC v145, x64
- **테스트 프레임워크:** Google Mock 1.11.0 (`sk_car/packages/gmock.1.11.0`)
- **레거시 소스:** `cpp/assemble.cpp` (git history에서 복원 가능, 현재 삭제됨)

---

## 도메인 요약 (Day2.pdf p.20-23)

차량 제조 3단계:
1. **차량 타입 선택** — Sedan / SUV / Truck
2. **부품 조립** — Engine(GM·TOYOTA·WIA) + BrakeSystem(MANDO·CONTINENTAL·BOSCH) + SteeringSystem(BOSCH·MOBIS)
3. **호환성 검사** — 아래 5개 제한조건 충족 여부 확인

| # | 제한조건 |
|---|---------|
| 1 | BOSCH 제동장치 → BOSCH 조향장치 필수 |
| 2 | CONTINENTAL — Sedan용 제동장치 없음 |
| 3 | TOYOTA — SUV용 엔진 없음 |
| 4 | WIA — Truck용 엔진 없음 |
| 5 | MANDO — Truck용 제동장치 없음 |

---

## 레거시 코드 주요 문제점 (Day2.pdf p.24)

- 절차지향 구조, 유지보수 어려움
- `int stack[10]` 전역 배열로 상태 관리 (타입 안전성 없음)
- C-style 문법 (`printf`, `fgets`, `strtok_s`, busy-wait `delay()`)
- 확장성 없음 — 새 차량 타입·부품 추가 시 기존 코드 전체 수정 필요
- `isValidCheck()` / `testProducedCar()` 로직 중복 (DRY 위반)
- 버그: `selectSteeringSystem()` 출력 문자열 오류, `Mando` 출력 `\n` 누락
- 유닛 테스트 없음

---

## 리팩터링 방향

**핵심 원칙: OCP (Open/Closed Principle)**  
새 차량 타입·부품·호환성 규칙 추가 시 기존 코드 수정 없이 새 클래스 추가만으로 완결

**4개 도메인을 각각 인터페이스로 추상화:**
- `ICarType` → Sedan / SUV / Truck
- `IEngine` → GmEngine / ToyotaEngine / WiaEngine / BrokenEngine
- `IBrakeSystem` → MandoBrakeSystem / ContinentalBrakeSystem / BoschBrakeSystem
- `ISteeringSystem` → BoschSteeringSystem / MobisSteeringSystem

**호환성 규칙도 인터페이스화:**
- `ICompatibilityRule` → 규칙별 독립 클래스 5개
- `Validator`는 규칙 목록을 순회할 뿐, 규칙 내용을 직접 알지 않음

---

## 진행 상태

| Phase | 내용 | 상태 |
|-------|------|------|
| Phase 1 | Method level — 버그 수정, C-style → C++, delay 교체, DRY 적용 | 계획 완료 |
| Phase 2 | 데이터 구조 — 전역 배열 제거, enum class, main() 분해 | 계획 완료 |
| Phase 3 | Class / OCP — 4개 인터페이스 + Validator + ConsoleUI + CarAssembler | 계획 완료 |
| Phase 4 | 유닛 테스트 — TC 코드 작성, 기능 단위 파일 분리 | 계획 완료 |

---

## 상세 리팩터링 계획

→ **[temp_docs/PLAN.md](temp_docs/PLAN.md)** 참조

PLAN.md 포함 내용:
- Phase별 세부 작업 항목 및 코드 예시
- 최종 파일 구조 (`cpp/interfaces/`, `cpp/engine/`, `tests/` 등)
- TC 코드 전문 (TC-1 ~ TC-3, Google Test Fixture 포함)
- OCP 확장 시나리오 표

---

## 파일 구조 (리팩터링 완료 후 목표)

```
cpp/
├── interfaces/         ICarType, IEngine, IBrakeSystem, ISteeringSystem, ICompatibilityRule
├── car_type/           Sedan, SUV, Truck
├── engine/             GmEngine, ToyotaEngine, WiaEngine, BrokenEngine
├── brake_system/       MandoBrakeSystem, ContinentalBrakeSystem, BoschBrakeSystem
├── steering_system/    BoschSteeringSystem, MobisSteeringSystem
├── rules/              호환성 규칙 5개 (규칙별 독립 파일)
├── Car.h
├── Validator.h/cpp
├── ConsoleUI.h/cpp
├── CarAssembler.h/cpp
└── main.cpp

tests/
├── test_engine.cpp
├── test_car.cpp
├── test_validator.cpp
├── test_car_type.cpp
├── test_brake_system.cpp
└── test_steering_system.cpp
```
