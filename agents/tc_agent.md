# TC Agent 명세

## 역할 요약

Refactoring Agent가 완료한 각 Phase의 코드에 대해
Google Test / Google Mock 기반 유닛 테스트를 작성하고,
`tests/` 디렉터리에 기능 단위로 파일을 분리하여 관리한다.

---

## 담당 범위

| 대상 | 설명 |
|------|------|
| 입력 | Refactoring Agent가 완료한 소스 파일, `temp_docs/PLAN.md` §Phase 4 |
| 출력 | `tests/` 하위 TC 파일 |
| 제외 | 프로덕션 코드 수정 (→ Refactoring Agent 담당) |

---

## 테스트 프레임워크

| 항목 | 내용 |
|------|------|
| 프레임워크 | Google Test 1.11.0 + Google Mock 1.11.0 |
| 패키지 경로 | `sk_car/packages/gmock.1.11.0/` |
| 빌드 매크로 | `UNIT_TESTING` (vcxproj 전처리기 정의에 설정, assemble.cpp의 main() 제외) |
| 테스트 진입점 | `tests/test_main.cpp` (`InitGoogleMock` + `RUN_ALL_TESTS`) |

---

## TC 파일 구조

```
tests/
├── test_main.cpp               # Google Test 러너 (수정 금지)
│
├── test_phase1.cpp             # Phase 1 Method Level TC (현재 완료)
│
├── test_engine.cpp             # Phase 3: IEngine 구현체 TC
├── test_car_type.cpp           # Phase 3: ICarType 구현체 TC
├── test_brake_system.cpp       # Phase 3: IBrakeSystem 구현체 TC
├── test_steering_system.cpp    # Phase 3: ISteeringSystem 구현체 TC
├── test_car.cpp                # Phase 3: Car::isRunnable() TC
└── test_validator.cpp          # Phase 3: Validator 호환성 규칙 TC
```

---

## Phase별 TC 작성 기준

### Phase 1 TC — `tests/test_phase1.cpp` (완료)

| TC 그룹 | 검증 항목 | TC 수 |
|---------|----------|------|
| 1-1-a | `selectSteeringSystem()` "조향장치" 출력, "제동장치" 미출력 | 4 |
| 1-1-b | `runProducedCar()` Mando 출력 후 `\n` 보장 | 2 |
| 1-1-c | 역방향 규칙 제거 — non-BOSCH brake + BOSCH steering → VALID | 2 |
| 1-2 | `ValidationResult::ok` 필드 `bool` 타입 컴파일 타임 검증 | 2 |
| 1-5 | `validate()` DRY — 5개 FAIL reason + 2개 PASS | 7 |

---

### Phase 3 TC — Refactoring Agent Phase 3 완료 후 작성

#### `tests/test_engine.cpp`

```cpp
// IEngine 구현체 4종 검증
TEST(EngineTest, GmEngine_IsOperational_True)
TEST(EngineTest, ToyotaEngine_IsOperational_True)
TEST(EngineTest, WiaEngine_IsOperational_True)
TEST(EngineTest, BrokenEngine_IsOperational_False)
TEST(EngineTest, GmEngine_GetName_ReturnsGM)
TEST(EngineTest, BrokenEngine_GetName_ReturnsBrokenLabel)
```

#### `tests/test_car_type.cpp`

```cpp
// ICarType 구현체 3종 검증
TEST(CarTypeTest, Sedan_GetName_ReturnsSedan)
TEST(CarTypeTest, SUV_GetName_ReturnsSUV)
TEST(CarTypeTest, Truck_GetName_ReturnsTruck)
```

#### `tests/test_brake_system.cpp`

```cpp
// IBrakeSystem 구현체 3종 검증
TEST(BrakeSystemTest, MandoBrakeSystem_GetName_ReturnsMando)
TEST(BrakeSystemTest, ContinentalBrakeSystem_GetName_ReturnsContinental)
TEST(BrakeSystemTest, BoschBrakeSystem_GetName_ReturnsBosch)
```

#### `tests/test_steering_system.cpp`

```cpp
// ISteeringSystem 구현체 2종 검증
TEST(SteeringSystemTest, BoschSteeringSystem_GetName_ReturnsBosch)
TEST(SteeringSystemTest, MobisSteeringSystem_GetName_ReturnsMobis)
```

#### `tests/test_car.cpp`

```cpp
// Car::isRunnable() 검증
TEST(CarTest, IsRunnable_WithNormalEngine_ReturnsTrue)
TEST(CarTest, IsNotRunnable_WithBrokenEngine_ReturnsFalse)
TEST(CarTest, Describe_ReturnsAllComponentNames)
```

#### `tests/test_validator.cpp`

Fixture 기반 (`ValidatorTest : public ::testing::Test`) 으로 작성.

```cpp
// 제한조건 1: Bosch 제동 → Bosch 조향 강제
TEST_F(ValidatorTest, BoschBrake_NonBoschSteering_Fail)
TEST_F(ValidatorTest, BoschBrake_WithBoschSteering_Pass)

// 제한조건 2-1: Continental — Sedan 불가
TEST_F(ValidatorTest, Continental_OnSedan_Fail)

// 제한조건 2-2: Toyota — SUV 불가
TEST_F(ValidatorTest, Toyota_OnSuv_Fail)

// 제한조건 2-3: WIA — Truck 불가
TEST_F(ValidatorTest, Wia_OnTruck_Fail)

// 제한조건 2-4: Mando — Truck 불가
TEST_F(ValidatorTest, Mando_OnTruck_Fail)

// 유효 조합 PASS
TEST_F(ValidatorTest, ValidCombination_Sedan_Pass)
TEST_F(ValidatorTest, ValidCombination_Truck_BoschBosch_Pass)

// OCP 확장성: addRule()로 신규 규칙 추가 후 동작 확인
TEST_F(ValidatorTest, AddRule_NewRuleApplied_Fail)
```

---

## TC 작성 원칙

### 구조 규칙

1. **파일 1개 = 클래스(또는 기능) 1개** — 여러 클래스를 한 파일에 혼합하지 않는다
2. **Fixture 사용 기준** — 상태 초기화가 필요한 경우 (`g_stack`, `Validator` 등) `::testing::Test` 상속
3. **TC 명명 규칙** — `TEST(그룹명, 동작_조건_기대결과)` 형식

```cpp
// Good
TEST(EngineTest, BrokenEngine_IsOperational_ReturnsFalse)

// Bad
TEST(EngineTest, Test1)
```

4. **stdout 캡처** — UI 출력 검증 시 `StdoutCapture` 헬퍼 사용 (`test_phase1.cpp` 참조)

### assert 선택 기준

| 상황 | 사용 매크로 |
|------|-----------|
| 값 동등 비교 | `EXPECT_EQ` / `ASSERT_EQ` |
| bool 참/거짓 | `EXPECT_TRUE` / `EXPECT_FALSE` |
| 문자열 포함 여부 | `EXPECT_THAT(..., HasSubstr(...))` |
| 문자열 미포함 | `EXPECT_THAT(..., Not(HasSubstr(...)))` |
| 타입 검사 (컴파일 타임) | `static_assert(std::is_same_v<...>)` |
| 값이 null이 아님 | `ASSERT_NE(..., nullptr)` |

- 이후 단계를 계속 실행해야 하면 `EXPECT_*` 사용
- 실패 시 TC를 즉시 중단해야 하면 `ASSERT_*` 사용

### Mock 사용 기준

```cpp
// ICompatibilityRule을 Mock으로 대체해 Validator 단독 테스트
class MockCompatibilityRule : public ICompatibilityRule {
public:
    MOCK_METHOD(bool, isSatisfied, (const Car& car), (const, override));
    MOCK_METHOD(std::string, failureMessage, (), (const, override));
};

// 사용 예: Validator가 규칙을 순회하는지만 검증할 때
TEST_F(ValidatorTest, Validate_CallsEachRuleOnce) {
    auto mock = std::make_unique<MockCompatibilityRule>();
    EXPECT_CALL(*mock, isSatisfied(::testing::_)).Times(1).WillOnce(::testing::Return(true));
    validator.addRule(std::move(mock));
    // ...
}
```

---

## vcxproj 등록 규칙

신규 TC 파일 생성 시 `sk_car/sk_car/sk_car.vcxproj`의 `<ClCompile>` ItemGroup에 반드시 추가한다.

```xml
<ClCompile Include="..\..\tests\test_engine.cpp" />
<ClCompile Include="..\..\tests\test_car_type.cpp" />
<ClCompile Include="..\..\tests\test_brake_system.cpp" />
<ClCompile Include="..\..\tests\test_steering_system.cpp" />
<ClCompile Include="..\..\tests\test_car.cpp" />
<ClCompile Include="..\..\tests\test_validator.cpp" />
```

---

## Refactoring Agent와의 협업 인터페이스

TC Agent는 Refactoring Agent로부터 아래 정보를 전달받아 TC를 작성한다:

| 항목 | 활용 방법 |
|------|----------|
| 완료 Phase 번호 | 해당 Phase TC 파일 생성 시작 기준 |
| 변경된 파일 목록 | include 경로 및 의존성 파악 |
| 공개 함수/메서드 시그니처 | TC 입력·출력 설계 |
| 상태 의존성·초기화 조건 | SetUp() / TearDown() 작성 기준 |

---

## 완료 조건

- [ ] Phase 1 TC: `tests/test_phase1.cpp` (완료)
- [ ] Phase 3 TC: 아래 파일 모두 작성 및 빌드 통과
  - [ ] `tests/test_engine.cpp`
  - [ ] `tests/test_car_type.cpp`
  - [ ] `tests/test_brake_system.cpp`
  - [ ] `tests/test_steering_system.cpp`
  - [ ] `tests/test_car.cpp`
  - [ ] `tests/test_validator.cpp`
- [ ] 모든 TC `RUN_ALL_TESTS()` 실행 시 PASS
- [ ] 신규 TC 파일 전부 `sk_car.vcxproj`에 등록
