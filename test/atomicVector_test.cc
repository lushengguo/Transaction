#include "atom.h"
#include <gtest/gtest.h>

template <typename... Args>
bool equal(const std::vector<int> &vec, Args... args)
{
    bool res = true;
    size_t index = 0;
    auto equal = [&](const auto i) { res &= vec.size() > index ? vec[index++] == i : false; };
    (equal(args), ...);
    return res;
}

template <typename... Args>
bool equal(const AtomIntVector &vec, Args... args)
{
    const AtomIntVector::ValueType &raw = vec.get();
    return equal(raw, std::forward<Args>(args)...);
}

TEST(Equal, Interface)
{
    std::vector<int> v1{};
    EXPECT_TRUE(equal(v1));

    std::vector<int> v2{1, 2, 3};
    EXPECT_TRUE(equal(v2, 1, 2, 3));
    EXPECT_FALSE(equal(v2, 1, 2, 3, 4));
}

TEST(AtomIntVector, Init)
{
    AtomIntVector as(1, 0);
    EXPECT_FALSE(as.inTransaction());
    EXPECT_TRUE(equal(as, 0));
}

TEST(AtomIntVector, CommitModify)
{
    AtomIntVector as(1, 0);
    as.beginTransaction();
    {
        as.modify(AtomIntVector::ModifyType::Modify, 0, 1);
    }
    as.endTransaction();
    EXPECT_TRUE(equal(as, 1));
    EXPECT_FALSE(as.inTransaction());
}

TEST(AtomIntVector, RollbackModifyInRoot)
{
    AtomIntVector as(1, 0);
    as.beginTransaction();
    {
        as.modify(AtomIntVector::ModifyType::Modify, 0, 1);
    }
    as.endTransaction();
    EXPECT_TRUE(equal(as, 1));
    EXPECT_FALSE(as.inTransaction());

    as.undo();
    EXPECT_FALSE(as.inTransaction());
    EXPECT_TRUE(equal(as, 0));
}

TEST(AtomIntVector, RollbackInsertInRoot)
{
    AtomIntVector as(1, 0);
    as.beginTransaction();
    {
        as.modify(AtomIntVector::ModifyType::Insert, 0, 1);
    }
    as.endTransaction();
    EXPECT_TRUE(equal(as, 1, 0));
    EXPECT_FALSE(as.inTransaction());

    as.undo();
    EXPECT_FALSE(as.inTransaction());
    EXPECT_TRUE(equal(as, 0));
}

TEST(AtomIntVector, RollbackEraseInRoot)
{
    AtomIntVector as(1, 0);
    as.beginTransaction();
    {
        as.modify(AtomIntVector::ModifyType::Erase, 0);
    }
    as.endTransaction();
    EXPECT_TRUE(equal(as));
    EXPECT_FALSE(as.inTransaction());

    as.undo();
    EXPECT_FALSE(as.inTransaction());
    EXPECT_TRUE(equal(as, 0));
}

TEST(AtomIntVector, UndoRedoRecursively)
{
    AtomIntVector as(1, 0);
    as.beginTransaction();
    {
        as.modify(AtomIntVector::ModifyType::Insert, 0, 1);
        as.beginTransaction();
        {
            EXPECT_TRUE(equal(as, 1));
            as.modify(AtomIntVector::ModifyType::Insert, 0, 2);
            EXPECT_TRUE(equal(as, 2, 1));
            as.modify(AtomIntVector::ModifyType::Erase, 1);
            EXPECT_TRUE(equal(as, 2));
        }
        as.endTransaction();
        as.undo();
        EXPECT_TRUE(equal(as, 1));
        as.redo();
        EXPECT_TRUE(equal(as, 2));
    }
    as.endTransaction();
    as.undo();
    EXPECT_FALSE(as.inTransaction());
    EXPECT_TRUE(equal(as));
}