#pragma once

#include "atomicInterface.h"
#include <assert.h>
#include <iostream>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

static bool globalLogEnable = false;

#define LOG                                                                                                            \
    if (globalLogEnable)                                                                                               \
    std::cout

void disableLog()
{
    globalLogEnable = false;
}

void enableLog()
{
    globalLogEnable = true;
}

template <typename Tp>
class TransInterface
{
  public:
    typedef AtomInterface<Tp> ValueType;
    using ModifyRecord = typename ValueType::ModifyRecord;
    using ModifyType = typename ValueType::ModifyType;

  public:
    template <typename... Args>
    TransInterface(Args... args) : val_(std::forward<Args>(args)...), nextCommitId_(0)
    {
    }

  public:
    class Commit;
    enum class CommitTag
    {
        beginTrans,
        endTrans,
        undo,
        redo
    };

    typedef size_t CommitId;
    typedef std::vector<std::shared_ptr<Commit>> Commits;

    struct Commit
    {
        CommitTag tag_;
        CommitId id_;
        std::vector<ModifyRecord> modifyRecords_;
        std::shared_ptr<Commits> children_; // recursive transaction
        std::weak_ptr<Commit> parent_;
    };

    static constexpr size_t EmptyTransaction = std::numeric_limits<size_t>::max();

    std::shared_ptr<Commits> root_;
    std::shared_ptr<Commit> curCommit_;
    CommitId nextCommitId_;

  public:
    template <typename... Args>
    void modify(typename ValueType::ModifyType modifyType, Args... args)
    {
        assert(inTransaction());
        auto modifyRecord = val_.modify(modifyType, std::forward<Args>(args)...);
        curCommit_->modifyRecords_.emplace_back(std::move(modifyRecord));
    }

    const ValueType &get() const
    {
        return val_;
    }

    bool inTransaction()
    {
        return bool(curCommit_);
    }

    void beginTransaction()
    {
        std::shared_ptr<Commit> newCommit(new Commit());
        newCommit->tag_ = CommitTag::beginTrans;
        newCommit->id_ = nextCommitId_++;
        newCommit->parent_ = curCommit_;
        LOG << currentLayerLogPrefix(newCommit) << "begin transaction, CommitId=" << newCommit->id_ << std::endl;

        if (!curCommit_)
        {
            if (!root_)
                root_ = std::make_shared<Commits>();
            root_->emplace_back(newCommit);
            curCommit_ = newCommit;
            return;
        }

        if (!curCommit_->children_)
        {
            curCommit_->children_ = std::make_shared<Commits>();
        }

        curCommit_->children_->emplace_back(newCommit);
        curCommit_ = newCommit;
    }

    size_t endTransaction()
    {
        if (!inTransaction())
            return EmptyTransaction;

        std::vector<ModifyRecord> &modifyRecords = curCommit_->modifyRecords_;
        CommitId id = curCommit_->id_;
        curCommit_->tag_ = CommitTag::endTrans;
        LOG << currentLayerLogPrefix(curCommit_) << "end transaction, CommitId=" << id
            << " modifyRecord:" << val_.serialModifyRecords(modifyRecords) << std::endl;
        curCommit_ = curCommit_->parent_.lock();
        return id;
    }

    void undo()
    {
        if (curCommit_)
        {
            if (curCommit_->children_->empty())
                return;

            undo(findUndoCommit(curCommit_->children_));
            return;
        }

        if (!root_->empty())
        {
            undo(findUndoCommit(root_));
        }
    }

    void redo()
    {
        if (curCommit_)
        {
            if (curCommit_->children_->empty())
                return;

            redo(findRedoCommit(curCommit_->children_));
            return;
        }

        if (!root_->empty())
        {
            redo(findRedoCommit(root_));
        }
    }

  private:
    void undo(std::shared_ptr<Commit> commit)
    {
        if (!commit)
            return;

        assert(commit->tag_ == CommitTag::endTrans);
        LOG << currentLayerLogPrefix(commit) << "undo transaction, CommitId=" << commit->id_ << std::endl;

        auto children = commit->children_;
        if (children)
        {
            while (auto undoCommit = findUndoCommit(children))
            {
                undo(undoCommit);
            }
        }
        std::shared_ptr<Commit> newCommit(new Commit());
        newCommit->tag_ = CommitTag::undo;
        newCommit->id_ = nextCommitId_++;
        newCommit->parent_ = commit->parent_;
        // 把commit的modifyRecord倒着跑一遍
        LOG << currentLayerLogPrefix(commit) << "undo modifyRecord:" << val_.serialModifyRecords(commit->modifyRecords_)
            << std::endl;
        for (auto riter = commit->modifyRecords_.rbegin(); riter != commit->modifyRecords_.rend(); ++riter)
        {
            std::string oldStr = val_.serialSelf();
            auto &oldRecord = *riter;
            auto newRecord = val_.rollback(oldRecord);
            newCommit->modifyRecords_.emplace_back(std::move(newRecord));
            LOG << currentLayerLogPrefix(commit) << "undo modifyRecord, oldVal=" << oldStr
                << ", newVal=" << val_.serialSelf() << std::endl;
        }

        auto parent = commit->parent_.lock();
        if (parent)
        {
            parent->children_->emplace_back(newCommit);
        }
        else
        {
            root_->emplace_back(newCommit);
        }
    }

    void redo(std::shared_ptr<Commit> commit)
    {
        if (!commit)
            return;

        assert(commit->tag_ == CommitTag::undo);
        LOG << currentLayerLogPrefix(commit) << "redo transaction, CommitId=" << commit->id_ << std::endl;

        auto children = commit->children_;
        if (children)
        {
            while (auto redoCommit = findRedoCommit(children))
            {
                redo(redoCommit);
            }
        }

        std::shared_ptr<Commit> newCommit(new Commit());
        newCommit->tag_ = CommitTag::redo;
        newCommit->id_ = nextCommitId_++;
        newCommit->parent_ = commit->parent_;
        // 把commit的modifyRecord倒着跑一遍
        LOG << currentLayerLogPrefix(commit) << "undo modifyRecord:" << val_.serialModifyRecords(commit->modifyRecords_)
            << std::endl;
        for (auto riter = commit->modifyRecords_.rbegin(); riter != commit->modifyRecords_.rend(); ++riter)
        {
            std::string oldStr = val_.serialSelf();
            auto &oldRecord = *riter;
            auto newRecord = val_.rollback(oldRecord);
            newCommit->modifyRecords_.emplace_back(std::move(newRecord));
            LOG << currentLayerLogPrefix(commit) << "undo modifyRecord, oldVal=" << oldStr
                << ", newVal=" << val_.serialSelf() << std::endl;
        }

        auto parent = commit->parent_.lock();
        if (parent)
        {
            parent->children_->emplace_back(newCommit);
        }
        else
        {
            root_->emplace_back(newCommit);
        }
    }

    // not-finished-commit -> nullptr
    // commit -> commit
    // commit undo -> nullptr
    // commit undo redo -> commit
    // commit undo undo redo -> nullptr
    std::shared_ptr<Commit> findUndoCommit(std::shared_ptr<Commits> commits)
    {
        if (!commits)
            return nullptr;

        LOG << currentLayerLogPrefix(commits) << "findUndoCommit:: " << serialCommits(commits) << std::endl;
        size_t undoCnt = 0;
        for (auto iter = commits->rbegin(); iter != commits->rend(); ++iter)
        {
            if (!*iter)
                continue;
            CommitTag tag = (*iter)->tag_;
            switch (tag)
            {
            case CommitTag::beginTrans:
                return nullptr;
            case CommitTag::endTrans: {
                if (!undoCnt)
                    return *iter;
                undoCnt--;
                break;
            }
            case CommitTag::undo:
                undoCnt++;
                break;
            case CommitTag::redo:
                undoCnt--;
                break;
            default:
                assert(false);
            }
        }
        return nullptr;
    }

    // not-finished-commit -> nullptr
    // empty -> nullptr
    // commit commit -> nullptr
    // undo commit -> nullptr
    // commit undo -> undo
    // commit undo1 undo2 -> undo2
    // commit undo redo -> undo
    // commit commit commit undo1 undo2 undo3 redo3 redo2 -> undo1
    std::shared_ptr<Commit> findRedoCommit(std::shared_ptr<Commits> commits)
    {
        if (!commits)
            return nullptr;

        LOG << currentLayerLogPrefix(commits) << "findRedoCommit:: " << serialCommits(commits) << std::endl;

        size_t redoCnt = 0;
        for (auto iter = commits->rbegin(); iter != commits->rend(); ++iter)
        {
            if (!*iter)
                continue;

            switch ((*iter)->tag_)
            {
            case CommitTag::beginTrans:
            case CommitTag::endTrans:
                return nullptr;
            case CommitTag::undo: {
                if (!redoCnt)
                    return *iter;
                redoCnt--;
                break;
            }
            case CommitTag::redo: {
                redoCnt++;
                break;
            }
            default:
                assert(false);
            }
        }

        return nullptr;
    }

    std::string serialCommits(std::shared_ptr<Commits> commits)
    {
        if (!commits)
            return "";

        std::ostringstream oss;
        for (auto &&commit : *commits)
        {
            switch (commit->tag_)
            {
            case CommitTag::beginTrans:
                oss << "not-committed";
                break;
            case CommitTag::endTrans:
                oss << "commit";
                break;
            case CommitTag::undo:
                oss << "undo";
                break;
            case CommitTag::redo:
                oss << "redo";
                break;
            default:
                break;
            }
            oss << "(" << commit->id_ << ") ";
        }
        return oss.str();
    }

    std::string currentLayerLogPrefix(std::shared_ptr<Commit> commit)
    {
        size_t currentLayerIndex = 0;
        while (commit->parent_.lock())
        {
            currentLayerIndex++;
            commit = commit->parent_.lock();
        }
        if (!currentLayerIndex)
            return "";
        return std::string(currentLayerIndex, '-') + std::string(" ");
    }

    std::string currentLayerLogPrefix(std::shared_ptr<Commits> commits)
    {
        if (!commits || (commits && commits->empty()))
            return "[Empty Commits, cannot locate layer]";

        return currentLayerLogPrefix(commits->front());
    }

  private:
    ValueType val_;
};