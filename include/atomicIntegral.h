#pragma once
#include "atomicInterface.h"
#include <cstddef>
#include <sstream>
#include <type_traits>

template <typename T>
concept Integral = std::is_integral_v<T>;

template <Integral T>
class AtomInterface<T>
{
  public:
    typedef T ValueType;
    enum class ModifyType
    {
        modify
    };

    struct ModifyRecord
    {
        ModifyRecord(T oldVal, T newVal) : oldVal_(oldVal), newVal_(newVal)
        {
        }

        ModifyRecord(ModifyRecord &&rhs) : oldVal_(rhs.oldVal_), newVal_(rhs.newVal_)
        {
        }
        T oldVal_;
        T newVal_;
    };

  public:
    AtomInterface(T t) : val_(t)
    {
    }

    ModifyRecord rollback(ModifyRecord &rec)
    {
        T oldVal = val_;
        val_ = rec.oldVal_;
        return ModifyRecord(rec.newVal_, rec.oldVal_);
    }

    template <Integral Input>
    ModifyRecord modify(ModifyType type, Input newVal)
    {
        T oldVal = val_;
        val_ = newVal;
        return {oldVal, newVal};
    }

    std::string serialModifyRecords(std::vector<ModifyRecord> &records) const
    {
        std::ostringstream oss;
        for (auto &&rec : records)
            oss << "{oldVal=" << rec.oldVal_ << ", newVal=" << rec.newVal_ << "}";
        return oss.str();
    }

    std::string serialSelf() const
    {
        return std::to_string(val_);
    }

    const T &getRaw() const
    {
        return val_;
    }

  private:
    T val_;
};