#include "atom.h"

int main()
{
    // test AtomSize_t
    {
        AtomSize_t in;
        in.beginTransaction();
        in.modify(AtomSize_t::ModifyType::modify, size_t(1));
        in.endTransaction();
        assert(in.get().getRaw() == size_t(1));

        in.beginTransaction();
        {
            in.beginTransaction();
            in.modify(AtomSize_t::ModifyType::modify, size_t(1));
            in.endTransaction();

            in.beginTransaction();
            in.modify(AtomSize_t::ModifyType::modify, size_t(2));
        }

        {
            in.beginTransaction();
            in.modify(AtomSize_t::ModifyType::modify, size_t(3));
            in.endTransaction();
        }
        in.endTransaction();
        in.undo();

        assert(!in.inTransaction());
        assert(in.get().getRaw() == size_t(1));
    }
}