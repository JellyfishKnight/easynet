#include "print.hpp"
#include <gtest/gtest.h>

class PrintTest: public ::testing::Test {
protected:
    void SetUp() {}

    void TearDown() {}
};

TEST_F(PrintTest, BasicTypeTest) {
    utils::print(1);
    utils::print(1.2);
    std::string s = "string";
    utils::print(s);
    utils::print("string");
    utils::print(std::vector<int>(5, 1));
    int* a = new int[5];
    for (int i = 0; i < 5; ++i) {
        a[i] = i;
    }
    utils::print(a, 5);
}

int main() {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}