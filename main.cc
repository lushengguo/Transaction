#include <assert.h>
#include <iostream>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

class AtomInt
{
  public:
    AtomInt() : val_(0), nextCommitId_(0)
    {
    }

    AtomInt(int val) : val_(val), nextCommitId_(0)
    {
    }

  public:
    struct Modify
    {
        int oldVal_, newVal_;
    };

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
        std::vector<Modify> steps_;
        std::shared_ptr<Commits> children_; // recursive transaction
        std::weak_ptr<Commit> parent_;
    };

    static constexpr size_t EmptyTransaction = std::numeric_limits<size_t>::max();

    std::shared_ptr<Commits> root_;
    std::shared_ptr<Commit> curCommit_;
    CommitId nextCommitId_;

  public:
    int &operator=(int val)
    {
        assert(inTransaction());
        curCommit_->steps_.emplace_back(Modify{val_, val});
        val_ = val;
        return val_;
    }

    int get() const
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
        std::cout << currentLayerLogPrefix(newCommit) << "begin transaction, CommitId=" << newCommit->id_ << std::endl;

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

        std::vector<Modify> &steps = curCommit_->steps_;
        CommitId id = curCommit_->id_;
        curCommit_->tag_ = CommitTag::endTrans;
        std::cout << currentLayerLogPrefix(curCommit_) << "end transaction, CommitId=" << id
                  << " steps:" << serialSteps(steps) << std::endl;
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
        std::cout << currentLayerLogPrefix(commit) << "undo transaction, CommitId=" << commit->id_ << std::endl;

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
        // 把commit的steps倒着跑一遍
        std::cout << currentLayerLogPrefix(commit) << "undo steps:" << serialSteps(commit->steps_) << std::endl;
        for (auto riter = commit->steps_.rbegin(); riter != commit->steps_.rend(); ++riter)
        {
            auto &step = *riter;
            newCommit->steps_.emplace_back(Modify{step.newVal_, step.oldVal_});
            assert(val_ == step.newVal_);
            val_ = step.oldVal_;
            std::cout << currentLayerLogPrefix(commit) << "undo step, oldVal=" << step.oldVal_
                      << ", newVal=" << step.newVal_ << ", change val to " << val_ << std::endl;
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
        std::cout << currentLayerLogPrefix(commit) << "redo transaction, CommitId=" << commit->id_ << std::endl;

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
        // 把undo的steps倒着跑一遍
        std::cout << currentLayerLogPrefix(commit) << "redo steps:" << serialSteps(commit->steps_) << std::endl;
        for (auto riter = commit->steps_.rbegin(); riter != commit->steps_.rend(); ++riter)
        {
            auto &step = *riter;
            newCommit->steps_.emplace_back(Modify{step.newVal_, step.oldVal_});
            assert(val_ == step.newVal_);
            val_ = step.oldVal_;
            std::cout << currentLayerLogPrefix(commit) << "redo step, oldVal=" << step.oldVal_
                      << ", newVal=" << step.newVal_ << ", change val to " << val_ << std::endl;
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

        std::cout << currentLayerLogPrefix(commits) << "findUndoCommit:: " << serialCommits(commits) << std::endl;
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

        std::cout << currentLayerLogPrefix(commits) << "findRedoCommit:: " << serialCommits(commits) << std::endl;

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

    std::string serialSteps(std::vector<Modify> &steps)
    {
        std::ostringstream oss;
        for (auto &&step : steps)
            oss << "{" << step.oldVal_ << ", " << step.newVal_ << "} ";
        return oss.str();
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
    int val_;
};

int main()
{
    size_t counter = 123;
    AtomInt in;
    in.beginTransaction();
    {
        size_t genTrans = 5;
        for (size_t transCnt = genTrans; transCnt; transCnt--)
        {
            in.beginTransaction();
            {
                for (size_t genStep = 5; genStep; genStep--)
                {
                    in = counter++;
                }
            }
        }
        for (size_t transCnt = genTrans; transCnt; transCnt--)
            in.endTransaction();
    }
    in.endTransaction();
    assert(!in.inTransaction());
    in.undo();

    assert(in.get() == 0);
}