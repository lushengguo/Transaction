#pragma once

#pragma once
#include "atomicInterface.h"
#include <cstddef>
#include <sstream>
#include <type_traits>
#include <vector>

template <typename T>
class AtomInterface<std::vector<T>>
{
  public:
    typedef std::vector<T> ValueType;
    enum class ModifyType
    {
        Fail,
        Modify,
        Insert,
        Erase
    };

    const char *stringfyModifyType(ModifyType type) const
    {
        switch (type)
        {
        case ModifyType::Fail:
            return "Fail";
        case ModifyType::Modify:
            return "Modify";
        case ModifyType::Insert:
            return "Insert";
        case ModifyType::Erase:
            return "Erase";
        default:
            return "Unknown";
        }
    }

    struct ModifyRecord
    {
        size_t offset_;
        ModifyType type_;
        T oldVal_;
        T newVal_;
    };

  public:
    template <typename... Args>
    AtomInterface(Args... args) : val_(std::forward<Args>(args)...)
    {
    }

    ModifyRecord rollback(ModifyRecord &rec)
    {
        switch (rec.type_)
        {
        case ModifyType::Modify:
            val_[rec.offset_] = rec.oldVal_;
            return ModifyRecord{rec.offset_, ModifyType::Modify, rec.newVal_, rec.oldVal_};

        case ModifyType::Insert:
            val_.erase(val_.begin() + rec.offset_);
            return ModifyRecord{rec.offset_, ModifyType::Erase, rec.newVal_, rec.oldVal_};

        case ModifyType::Erase:
            val_.insert(val_.begin() + rec.offset_, rec.oldVal_);
            return ModifyRecord{rec.offset_, ModifyType::Insert, rec.newVal_, rec.oldVal_};

        default:
            return ModifyRecord{};
        }
        return ModifyRecord{};
    }

    ModifyRecord modify(ModifyType type, size_t offset)
    {
        T oldVal = val_[offset];
        val_.erase(val_.begin() + offset);
        return ModifyRecord{offset, ModifyType::Erase, oldVal, oldVal};
    }

    template <typename Input>
    ModifyRecord modify(ModifyType type, size_t offset, Input &&newVal)
    {
        if (offset > val_.size())
            return ModifyRecord{offset, ModifyType::Fail, newVal, newVal};

        switch (type)
        {
        case ModifyType::Modify: {
            T oldVal = val_[offset];
            val_[offset] = newVal;
            return ModifyRecord{offset, ModifyType::Modify, oldVal, newVal};
        }

        case ModifyType::Insert:
            val_.insert(val_.begin() + offset, newVal);
            return ModifyRecord{offset, ModifyType::Insert, newVal, newVal};

        default:
            assert(false);
        }
        return ModifyRecord{};
    }

    std::string serialModifyRecords(std::vector<ModifyRecord> &records) const
    {
        std::ostringstream oss;
        for (auto &&rec : records)
            oss << "{offset=" << rec.offset_ << ", ModifyType=" << stringfyModifyType(rec.type_)
                << ", oldVal=" << rec.oldVal_ << ", newVal=" << rec.newVal_ << "} ";
        return oss.str();
    }

    std::string serialSelf() const
    {
        std::ostringstream oss;
        oss << "{";
        for (auto &&e : val_)
            oss << e << " ";
        oss << "} ";
        return oss.str();
    }

    const ValueType &getRaw() const
    {
        return val_;
    }

  private:
    ValueType val_;
};