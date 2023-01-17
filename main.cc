#include "atom.h"

int main()
{
    // test AtomSize_t
    {
        AtomSize_t in;

        // test normal undo
        in.beginTransaction();
        in.modify(AtomSize_t::ModifyType::modify, size_t(1));
        in.endTransaction();
        assert(in.get().getRaw() == size_t(1));

        in.beginTransaction();

        {
            // test nested undo didn't influnce parent
            in.beginTransaction();
            in.modify(AtomSize_t::ModifyType::modify, size_t(1));
            in.endTransaction();

            in.beginTransaction();
            in.modify(AtomSize_t::ModifyType::modify, size_t(2));
            in.endTransaction();
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