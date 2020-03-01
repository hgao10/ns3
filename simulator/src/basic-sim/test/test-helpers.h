#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include "ns3/test.h"

#define ASSERT_EQUAL(a, b) NS_TEST_ASSERT_MSG_EQ(a, b, "")
#define ASSERT_NOT_EQUAL(a, b) NS_TEST_ASSERT_MSG_NEQ(a, b, "")
#define ASSERT_TRUE(a) NS_TEST_ASSERT_MSG_EQ((a), true, "")
#define ASSERT_FALSE(a) NS_TEST_ASSERT_MSG_EQ((a), false, "")
#define ASSERT_EQUAL_APPROX(a, b, c) NS_TEST_ASSERT_MSG_EQ_TOL(a, b, c, "")

#endif //TEST_HELPERS_H
