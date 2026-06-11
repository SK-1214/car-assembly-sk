# Refactoring Agent 명세

## 역할 요약

`cpp/assemble.cpp`의 절차지향 레거시 코드를 `temp_docs/PLAN.md`에 정의된 Phase 순서에 따라
OCP(Open/Closed Principle) 기반 객체지향 구조로 리팩터링한다.

---

## 담당 범위

| 대상 | 설명 |
|------|------|
| 입력 | `cpp/assemble.cpp`, `cpp/assemble.h`, `temp_docs/PLAN.md` |
| 출력 | 각 Phase 완료 후 수정·생성된 소스 파일 |
| 제외 | TC 코드 작성 (→ TC Agent 담당) |

---

## 수행 Phase

### Phase 1 — Method Level
> 참조: `temp_docs/PLAN.md` §Phase 1

- [ ] **1-1** 버그 수정
  - `selectSteeringSystem()` 출력 문자열 오류 수정 (`제동장치` → `조향장치`)
  - `runProducedCar()` Mando 출력 `\n` 누락 수정
  - `isValidCheck()` 명세 외 역방향 규칙 제거
- [ ] **1-2** 함수 시그니처 정리
  - `isValidCheck()` 반환 타입 `int` → `bool`
  - unreachable `return true;` 제거
  - `selectbrakeSystem` → `selectBrakeSystem` (camelCase)
- [ ] **1-3** C-style 문법 → C++ 교체
  - `printf` → `std::cout`, `fgets`/`strtol` → `std::getline`/`std::stoi`
  - `strtok_s` (Windows 전용) → `std::string::erase`
  - C 헤더 → C++ 헤더
- [ ] **1-4** 독립 `if` 체인 → `if-else if` 정리
- [ ] **1-5** `testProducedCar()` + `isValidCheck()` 중복 제거
  - `ValidationResult { bool ok; std::string reason; }` 도입
  - `validate()` 단일 함수로 통합

### Phase 2 — 데이터 구조
> 참조: `temp_docs/PLAN.md` §Phase 2

- [ ] **2-1** 전역 `int stack[10]` → `CarSelection` 구조체
- [ ] **2-2** C `enum` → `enum class` (타입 안전성 확보, Phase 3 전 임시 단계)
- [ ] **2-3** `main()` 분해 → `readInput()` / `handleStep()` / `main()`

### Phase 3 — Class / OCP 인터페이스 설계
> 참조: `temp_docs/PLAN.md` §Phase 3

- [ ] **3-1** `ICarType` 인터페이스 + `Sedan` / `SUV` / `Truck` 구현체
- [ ] **3-2** `IEngine` 인터페이스 + `GmEngine` / `ToyotaEngine` / `WiaEngine` / `BrokenEngine`
- [ ] **3-3** `IBrakeSystem` 인터페이스 + `MandoBrakeSystem` / `ContinentalBrakeSystem` / `BoschBrakeSystem`
- [ ] **3-4** `ISteeringSystem` 인터페이스 + `BoschSteeringSystem` / `MobisSteeringSystem`
- [ ] **3-5** `Car` 클래스 (`unique_ptr<IXxx>` 조합)
- [ ] **3-6** `ICompatibilityRule` 인터페이스 + 규칙 5개 구현체 + `Validator`
- [ ] **3-7** `ConsoleUI` 클래스 (동적 메뉴 생성)
- [ ] **3-8** `CarAssembler` 클래스 (조립 흐름 제어)
- [ ] Phase 2의 `enum class` 전부 제거

---

## 파일 생성 규칙

### 목표 디렉터리 구조

```
cpp/
├── interfaces/
│   ├── ICarType.h
│   ├── IEngine.h
│   ├── IBrakeSystem.h
│   ├── ISteeringSystem.h
│   └── ICompatibilityRule.h
├── car_type/
│   ├── Sedan.h
│   ├── SUV.h
│   └── Truck.h
├── engine/
│   ├── GmEngine.h
│   ├── ToyotaEngine.h
│   ├── WiaEngine.h
│   └── BrokenEngine.h
├── brake_system/
│   ├── MandoBrakeSystem.h
│   ├── ContinentalBrakeSystem.h
│   └── BoschBrakeSystem.h
├── steering_system/
│   ├── BoschSteeringSystem.h
│   └── MobisSteeringSystem.h
├── rules/
│   ├── BoschBrakeRequiresBoschSteering.h
│   ├── ContinentalNotForSedan.h
│   ├── ToyotaNotForSuv.h
│   ├── WiaNotForTruck.h
│   └── MandoNotForTruck.h
├── Car.h
├── Validator.h / Validator.cpp
├── ConsoleUI.h / ConsoleUI.cpp
├── CarAssembler.h / CarAssembler.cpp
└── main.cpp
```

### 파일 생성 원칙

1. **인터페이스 파일 (`interfaces/`)은 구현체 헤더를 include 하지 않는다**
2. **단순 구현체 (getName() 등)는 header-only로 작성**
3. **복잡한 클래스 (Validator, ConsoleUI, CarAssembler)는 `.h` + `.cpp` 분리**
4. **각 파일은 하나의 클래스/인터페이스만 담는다**
5. **신규 파일 생성 시 `sk_car.vcxproj`의 `<ClCompile>` 또는 `<ClInclude>` 에 등록**

---

## 코드 작성 원칙

### OCP 준수 확인 기준

새로운 차량 타입·부품·호환 규칙 추가 시 아래 조건을 반드시 만족해야 한다:

| 확장 요구 | 수정 대상 | 기존 코드 수정 여부 |
|----------|----------|------------------|
| 새 차량 타입 | 신규 `.h` 파일 추가 | 없음 |
| 새 엔진/부품 | 신규 `.h` 파일 추가 | 없음 |
| 새 호환성 규칙 | 신규 `.h` + `Validator::addRule()` 호출 | 없음 |
| 메뉴 항목 | 없음 (ConsoleUI가 vector 순회) | 없음 |

### 금지 사항

- `enum`/`enum class` 기반 타입 분기 (`if type == SEDAN`) — Phase 3 이후 전면 금지
- `dynamic_cast` 남용 — 타입 판별 필요 시 인터페이스에 `virtual` 메서드 추가로 해결
- 전역 변수 — Phase 2 이후 모든 상태는 클래스 멤버로 관리
- C-style 문법 (`printf`, `malloc`, raw array) — Phase 1 완료 후 전면 금지

### 스타일 규칙

- C++20 표준 사용 (`std::unique_ptr`, range-for, structured bindings 활용)
- 소멸자는 항상 `virtual ~IXxx() = default;`로 선언
- 멤버 변수명은 `m_` 접두어 없이 camelCase 또는 snake_case 통일

---

## TC Agent와의 협업 인터페이스

Refactoring Agent가 각 Phase를 완료하면 TC Agent에 다음 정보를 전달한다:

| 항목 | 내용 |
|------|------|
| 완료 Phase | Phase 번호 및 변경된 파일 목록 |
| 테스트 가능 단위 | 공개 함수/메서드 시그니처 |
| 예상 동작 | 각 함수의 입력·출력 명세 |
| 주의 사항 | 상태 의존성, 초기화 조건 등 |

---

## 완료 조건

- [ ] 모든 Phase 1~3 항목 체크 완료
- [ ] `cpp/` 하위 목표 디렉터리 구조와 일치
- [ ] `sk_car.vcxproj`에 모든 신규 파일 등록
- [ ] 빌드 오류 없음 (Debug|x64 기준)
- [ ] 기존 동작 (차량 선택 → 조립 → 검증 플로우) 유지
