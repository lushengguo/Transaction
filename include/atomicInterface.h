#pragma once
#include <string>
#include <vector>

template <typename T>
class AtomInterface
{
  public:
    typedef T ValueType;
    enum class ModifyType;
    class ModifyRecord; // must support move constructor

  public:
    // rollback可以同时作用于undo/redo
    ModifyRecord rollback(ModifyRecord &);

    template <typename... Param>
    ModifyRecord modify(ModifyType, Param...);

    std::string serialModifyRecords(std::vector<ModifyRecord> &) const;
    std::string serialSelf() const;

    const ValueType &getRaw() const;

  private:
    ValueType val_;
};