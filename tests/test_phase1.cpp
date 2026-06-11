#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <sstream>
#include <cstring>
#include "../cpp/assemble.h"

// ─── stdout 캡처 헬퍼 ─────────────────────────────────────────────────────────
class StdoutCapture {
    std::streambuf* original_;
    std::ostringstream buf_;
public:
    StdoutCapture()  { original_ = std::cout.rdbuf(buf_.rdbuf()); }
    ~StdoutCapture() { std::cout.rdbuf(original_); }
    std::string get() const { return buf_.str(); }
};

// ─── 테스트 픽스처: 각 TC 실행 전 전역 상태 초기화 ──────────────────────────
class Phase1Test : public ::testing::Test {
protected:
    void SetUp() override {
        std::memset(g_stack, 0, sizeof(g_stack));
    }
};

// =============================================================================
// TC Group 1-1-a │ selectSteeringSystem() 출력 문자열 버그 수정
//   Before: "BOSCH 제동장치를 선택하셨습니다." (오출력)
//   After : "BOSCH 조향장치를 선택하셨습니다." (정상)
// =============================================================================

TEST_F(Phase1Test, TC_1_1_a_1_Bosch_Steering_OutputContains_조향장치) {
    StdoutCapture cap;
    selectSteeringSystem(BOSCH_S);
    EXPECT_THAT(cap.get(), ::testing::HasSubstr("조향장치"));
}

TEST_F(Phase1Test, TC_1_1_a_2_Bosch_Steering_OutputNotContains_제동장치) {
    StdoutCapture cap;
    selectSteeringSystem(BOSCH_S);
    EXPECT_THAT(cap.get(), ::testing::Not(::testing::HasSubstr("제동장치")));
}

TEST_F(Phase1Test, TC_1_1_a_3_Mobis_Steering_OutputContains_조향장치) {
    StdoutCapture cap;
    selectSteeringSystem(MOBIS);
    EXPECT_THAT(cap.get(), ::testing::HasSubstr("조향장치"));
}

TEST_F(Phase1Test, TC_1_1_a_4_Mobis_Steering_OutputNotContains_제동장치) {
    StdoutCapture cap;
    selectSteeringSystem(MOBIS);
    EXPECT_THAT(cap.get(), ::testing::Not(::testing::HasSubstr("제동장치")));
}

// =============================================================================
// TC Group 1-1-b │ runProducedCar() — Mando 출력 \n 누락 수정
//   Before: printf("Brake System : Mando");   ← \n 없음
//   After : std::cout << "Brake    : Mando\n" ← \n 보장
// =============================================================================

TEST_F(Phase1Test, TC_1_1_b_1_MandoBrake_OutputLine_EndsWithNewline) {
    g_stack[CarType_Q]       = SEDAN;
    g_stack[Engine_Q]        = GM;
    g_stack[BrakeSystem_Q]   = MANDO;
    g_stack[SteeringSystem_Q] = MOBIS;

    StdoutCapture cap;
    runProducedCar();

    std::string out = cap.get();
    size_t mando_pos = out.find("Mando");
    ASSERT_NE(mando_pos, std::string::npos) << "출력에 'Mando' 가 없음";

    // "Mando" 이후 첫 번째 문자가 \n 이어야 함
    size_t newline_pos   = out.find('\n', mando_pos);
    size_t steering_pos  = out.find("Steering", mando_pos);
    EXPECT_LT(newline_pos, steering_pos)
        << "Mando 출력 줄에 \\n 이 없어서 Steering 출력보다 뒤에 개행이 나옴";
}

TEST_F(Phase1Test, TC_1_1_b_2_MandoBrake_OutputContains_MandonNewline) {
    g_stack[CarType_Q]       = SEDAN;
    g_stack[Engine_Q]        = GM;
    g_stack[BrakeSystem_Q]   = MANDO;
    g_stack[SteeringSystem_Q] = MOBIS;

    StdoutCapture cap;
    runProducedCar();

    EXPECT_THAT(cap.get(), ::testing::HasSubstr("Mando\n"));
}

// =============================================================================
// TC Group 1-1-c │ isValidCheck() 잘못된 역방향 규칙 제거
//   Before: (brakeSystem != BOSCH_B && steeringSystem == BOSCH_S) → false
//           → SEDAN + GM + MANDO + BOSCH_S 가 잘못 FAIL 처리됨
//   After : 해당 조건 제거 → 위 조합은 VALID (PASS)
// =============================================================================

TEST_F(Phase1Test, TC_1_1_c_1_NonBoschBrake_BoschSteering_IsValid) {
    // MANDO brake(non-BOSCH) + BOSCH steering → 명세상 유효한 조합
    g_stack[CarType_Q]       = SEDAN;
    g_stack[Engine_Q]        = GM;
    g_stack[BrakeSystem_Q]   = MANDO;       // non-BOSCH
    g_stack[SteeringSystem_Q] = BOSCH_S;    // BOSCH steering
    ValidationResult result = validate();
    EXPECT_TRUE(result.ok)
        << "non-BOSCH brake + BOSCH steering 은 유효한 조합이어야 함 (역방향 규칙 버그)";
}

TEST_F(Phase1Test, TC_1_1_c_2_ContinentalBrake_BoschSteering_IsValid) {
    // CONTINENTAL brake(non-BOSCH) + BOSCH steering on SUV → 유효
    g_stack[CarType_Q]       = TRUCK;
    g_stack[Engine_Q]        = GM;
    g_stack[BrakeSystem_Q]   = BOSCH_B;
    g_stack[SteeringSystem_Q] = BOSCH_S;
    ValidationResult result = validate();
    EXPECT_TRUE(result.ok);
}

// =============================================================================
// TC Group 1-2 │ validate() 반환 타입 — ok 필드가 bool 타입인지 컴파일 타임 확인
// =============================================================================

TEST_F(Phase1Test, TC_1_2_1_ValidateResult_OkField_IsBoolType) {
    static_assert(std::is_same_v<decltype(ValidationResult::ok), bool>,
                  "ValidationResult::ok 의 타입이 bool 이어야 함 (int 아님)");
    SUCCEED();
}

TEST_F(Phase1Test, TC_1_2_2_ValidCombination_ReturnsBoolTrue) {
    g_stack[CarType_Q]       = SEDAN;
    g_stack[Engine_Q]        = GM;
    g_stack[BrakeSystem_Q]   = MANDO;
    g_stack[SteeringSystem_Q] = MOBIS;
    ValidationResult result = validate();
    EXPECT_TRUE(result.ok);
}

// =============================================================================
// TC Group 1-5 │ validate() DRY 통합 — 실패 시 reason 문자열 포함 검증
//   Before: isValidCheck()(조건) + testProducedCar()(메시지) 로직 이중 관리
//   After : validate() 하나가 ok + reason 반환 → 단일 소스
// =============================================================================

TEST_F(Phase1Test, TC_1_5_1_Continental_OnSedan_ReturnsFalse_WithReason) {
    g_stack[CarType_Q]       = SEDAN;
    g_stack[Engine_Q]        = GM;
    g_stack[BrakeSystem_Q]   = CONTINENTAL;
    g_stack[SteeringSystem_Q] = MOBIS;
    ValidationResult result = validate();
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.reason.empty()) << "실패 시 reason 이 비어 있으면 안 됨";
    EXPECT_THAT(result.reason, ::testing::HasSubstr("Continental"));
}

TEST_F(Phase1Test, TC_1_5_2_Toyota_OnSuv_ReturnsFalse_WithReason) {
    g_stack[CarType_Q]       = SUV;
    g_stack[Engine_Q]        = TOYOTA;
    g_stack[BrakeSystem_Q]   = MANDO;
    g_stack[SteeringSystem_Q] = MOBIS;
    ValidationResult result = validate();
    EXPECT_FALSE(result.ok);
    EXPECT_THAT(result.reason, ::testing::HasSubstr("TOYOTA"));
}

TEST_F(Phase1Test, TC_1_5_3_Wia_OnTruck_ReturnsFalse_WithReason) {
    g_stack[CarType_Q]       = TRUCK;
    g_stack[Engine_Q]        = WIA;
    g_stack[BrakeSystem_Q]   = BOSCH_B;    // MANDO on TRUCK 도 invalid이므로 BOSCH 사용
    g_stack[SteeringSystem_Q] = BOSCH_S;
    ValidationResult result = validate();
    EXPECT_FALSE(result.ok);
    EXPECT_THAT(result.reason, ::testing::HasSubstr("WIA"));
}

TEST_F(Phase1Test, TC_1_5_4_Mando_OnTruck_ReturnsFalse_WithReason) {
    g_stack[CarType_Q]       = TRUCK;
    g_stack[Engine_Q]        = GM;
    g_stack[BrakeSystem_Q]   = MANDO;
    g_stack[SteeringSystem_Q] = MOBIS;
    ValidationResult result = validate();
    EXPECT_FALSE(result.ok);
    EXPECT_THAT(result.reason, ::testing::HasSubstr("Mando"));
}

TEST_F(Phase1Test, TC_1_5_5_BoschBrake_NonBoschSteering_ReturnsFalse_WithReason) {
    g_stack[CarType_Q]       = SEDAN;
    g_stack[Engine_Q]        = GM;
    g_stack[BrakeSystem_Q]   = BOSCH_B;
    g_stack[SteeringSystem_Q] = MOBIS;     // non-BOSCH steering
    ValidationResult result = validate();
    EXPECT_FALSE(result.ok);
    EXPECT_THAT(result.reason, ::testing::HasSubstr("Bosch"));
}

TEST_F(Phase1Test, TC_1_5_6_ValidCombination_ReturnsTrue_ReasonEmpty) {
    g_stack[CarType_Q]       = SEDAN;
    g_stack[Engine_Q]        = GM;
    g_stack[BrakeSystem_Q]   = MANDO;
    g_stack[SteeringSystem_Q] = MOBIS;
    ValidationResult result = validate();
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(result.reason.empty()) << "성공 시 reason 은 비어 있어야 함";
}

TEST_F(Phase1Test, TC_1_5_7_ValidCombination_Truck_BoschBosch_Pass) {
    g_stack[CarType_Q]       = TRUCK;
    g_stack[Engine_Q]        = GM;
    g_stack[BrakeSystem_Q]   = BOSCH_B;
    g_stack[SteeringSystem_Q] = BOSCH_S;
    EXPECT_TRUE(validate().ok);
}
