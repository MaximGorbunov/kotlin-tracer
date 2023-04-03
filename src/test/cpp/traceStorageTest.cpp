#include <gtest/gtest.h>
#include "traceStorage.h"

using namespace std;
TEST(TraceStorageTest, TestAddTrace) {
    TraceStorage storage;

    storage.addRawTrace(0,  make_shared<ASGCT_CallTrace>(), pthread_self());
    storage.addRawTrace(0,  make_shared<ASGCT_CallTrace>(), pthread_self());
    EXPECT_NE(storage.getHeadTrace()->next_node, nullptr);
}