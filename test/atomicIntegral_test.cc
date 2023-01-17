#include "atom.h"
#include <gtest/gtest.h>

TEST(atomIntegral, commit)
{
    AtomSize_t as;
    as.beginTransaction();
    as.modify(AtomSize_t::ModifyType::modify, size_t(1));
    as.endTransaction();
    EXPECT_FALSE(as.inTransaction());
    EXPECT_TRUE(as.get().getRaw() == 1);
}
