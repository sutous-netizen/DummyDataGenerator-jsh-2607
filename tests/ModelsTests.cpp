#include "Models.h"

#include <gtest/gtest.h>

using ddg::OrderStatus;
using ddg::ToString;

TEST(OrderStatusToStringTest, MapsEachStatusToExpectedLiteral) {
    EXPECT_EQ(ToString(OrderStatus::Reserved), "RESERVED");
    EXPECT_EQ(ToString(OrderStatus::Rejected), "REJECTED");
    EXPECT_EQ(ToString(OrderStatus::Producing), "PRODUCING");
    EXPECT_EQ(ToString(OrderStatus::Confirmed), "CONFIRMED");
    // Released -> "RELEASE" (RELEASED가 아님에 주의)
    EXPECT_EQ(ToString(OrderStatus::Released), "RELEASE");
}
