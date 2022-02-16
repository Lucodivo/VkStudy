#include "test.h"

namespace {
  class PlaygroundTest : public testing::Test {
  protected:
    // run immediately before a test starts
    void SetUp() override {

    }

    // invoked immediately after a test finishes
    void TearDown() override {

    }
  };

  // TEST_F is for using "fixtures"
  TEST(Playground, strncpys) {
    const char* butts = "butts";
    char copy[6] = {'?','?','?','?','?','?'};
    strncpy_s(copy, 6, butts, 5);
    ASSERT_EQ(strcmp(copy, butts), 0); // assert that it adds the null character
  }
}