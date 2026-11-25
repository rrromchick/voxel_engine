#include <gtest/gtest.h>
#include "Lightmap.hpp"

TEST(LightmapTest, StoresFourChannelsIndependently) {
    Lightmap lm;
    lm.set(2, 3, 4, 0, 15);
    lm.set(2, 3, 4, 1, 9);
    lm.set(2, 3, 4, 2, 4);
    lm.set(2, 3, 4, 3, 12);

    EXPECT_EQ(lm.get(2, 3, 4, 0), 15);
    EXPECT_EQ(lm.get(2, 3, 4, 1), 9);
    EXPECT_EQ(lm.get(2, 3, 4, 2), 4);
    EXPECT_EQ(lm.get(2, 3, 4, 3), 12);
}
