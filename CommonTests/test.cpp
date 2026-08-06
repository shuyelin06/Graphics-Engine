#include "pch.h"

#include "math/Vector3.h"

using namespace Engine::Math;

TEST(TestCaseName, TestName) {
	Vector3 test{};
	EXPECT_EQ(test.magnitude(), 0.f);
	EXPECT_EQ(1, 1);
	EXPECT_TRUE(true);
}