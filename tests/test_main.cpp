// UNIT_TESTING 매크로는 이 파일이 아닌 컴파일러 전처리 정의에서 설정해야 합니다.
//
// Visual Studio 프로젝트 설정:
//   [프로젝트 속성] → C/C++ → 전처리기 → 전처리기 정의에 UNIT_TESTING 추가
//
// CMake 사용 시:
//   target_compile_definitions(sk_car_test PRIVATE UNIT_TESTING)
//
// 이 설정이 있어야 assemble.cpp 의 main() 이 컴파일에서 제외되어
// 아래 테스트 main() 과 충돌하지 않습니다.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
