#pragma once
#include <string>
#include <vector>

template <typename ValueType>
class AtomInterface
{
  public:
    enum class ModifyType;
    class ModifyRecord; // must support move constructor

  public:
    // 前提: undo redo 可以操作抵消
    // undo作用于commit, redo作用于undo, 但他们本质上都是undo
    ModifyRecord undo(ModifyRecord &);

    template <typename... Param>
    ModifyRecord modify(ModifyType, Param...);

    std::string serialModifyRecords(std::vector<ModifyRecord> &) const;
    std::string serialSelf() const;

    const ValueType &getRaw() const;

  private:
    ValueType val_;
};