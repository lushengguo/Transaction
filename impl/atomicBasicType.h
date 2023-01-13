#pragma once
#include "atomicInterface.h"
#include <cstddef>
#include <sstream>

typedef AtomInterface<size_t> AISize_t;

template <>
enum class AISize_t::ModifyType {
    modify
};

template <>
struct AISize_t::ModifyRecord
{
    ModifyRecord(size_t oldVal, size_t newVal) : oldVal_(oldVal), newVal_(newVal)
    {
    }

    ModifyRecord(ModifyRecord &&rhs) : oldVal_(rhs.oldVal_), newVal_(rhs.newVal_)
    {
    }
    size_t oldVal_;
    size_t newVal_;
};

template <>
AISize_t::ModifyRecord AISize_t::undo(AISize_t::ModifyRecord &rec)
{
    size_t oldVal = val_;
    val_ = rec.oldVal_;
    return ModifyRecord(rec.newVal_, rec.oldVal_);
}

template <>
template <>
AISize_t::ModifyRecord AISize_t::modify<size_t>(AISize_t::ModifyType type, size_t newVal)
{
    size_t oldVal = val_;
    val_ = newVal;
    return {oldVal, newVal};
}

template <>
std::string AISize_t::serialModifyRecords(std::vector<ModifyRecord> &records) const
{
    std::ostringstream oss;
    for (auto &&rec : records)
        oss << "{oldVal=" << rec.oldVal_ << ", newVal=" << rec.newVal_ << "}";
    return oss.str();
}

template <>
std::string AISize_t::serialSelf() const
{
    return std::to_string(val_);
}

template <>
const size_t &AISize_t::getRaw() const
{
    return val_;
}
