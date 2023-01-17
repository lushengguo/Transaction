#include "atom.h"
#include <gtest/gtest.h>

TEST(AtomIntegral, Init)
{
    AtomInt as(0);
    EXPECT_FALSE(as.inTransaction());
    EXPECT_TRUE(as.get() == 0);
}

TEST(AtomIntegral, Commit)
{
    AtomInt as(0);
    as.beginTransaction();
    as.modify(AtomInt::ModifyType::modify, 1);
    as.endTransaction();
    EXPECT_FALSE(as.inTransaction());
    EXPECT_TRUE(as.get() == 1);
}

TEST(AtomIntegral, RollbackInRoot)
{
    AtomInt as(0);
    as.beginTransaction();
    as.modify(AtomInt::ModifyType::modify, 1);
    as.endTransaction();
    EXPECT_FALSE(as.inTransaction());
    EXPECT_TRUE(as.get() == 1);
    as.undo();
    EXPECT_TRUE(as.get() == 0);
}

TEST(AtomIntegral, EmptyRollbackInRoot)
{
    AtomInt as(0);
    as.beginTransaction();
    as.modify(AtomInt::ModifyType::modify, 1);
    as.endTransaction();
    EXPECT_FALSE(as.inTransaction());
    EXPECT_TRUE(as.get() == 1);
    as.undo();
    EXPECT_TRUE(as.get() == 0);
    as.undo();
    EXPECT_TRUE(as.get() == 0);
}

TEST(AtomIntegral, RecursiveRollBack)
{
    AtomInt as(0);
    as.beginTransaction();
    as.modify(AtomInt::ModifyType::modify, 2);
    {
        as.beginTransaction();
        as.modify(AtomInt::ModifyType::modify, 3);
        as.endTransaction();

        as.beginTransaction();
        as.modify(AtomInt::ModifyType::modify, 4);
        as.endTransaction();
    }
    as.endTransaction();

    EXPECT_FALSE(as.inTransaction());
    EXPECT_TRUE(as.get() == 4);
    as.undo();
    EXPECT_TRUE(as.get() == 0);
}
