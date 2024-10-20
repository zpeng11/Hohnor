/**
 * Binary heap implementation on c++ with template
 */
#pragma once
#include <type_traits>
#include <vector>
#include <functional>
#include "hohnor/common/Types.h"
#include "hohnor/common/Copyable.h"

/**
 * In logic, this is a binary heap with smallest-top, 
 * but you can use it as greatest-top by defining lessThanCmp.
 */
template <class T>
class BinaryHeap
{
private:
    std::vector<T> vec_;
    std::function<bool(T &, T &)> lessThanCmp_;
    inline size_t parent(size_t child) { return child ? (child - 1) >> 1 : child; }
    inline size_t lChild(size_t parent) { return (parent << 1) + 1; }
    inline size_t rChild(size_t parent) { return (parent + 1) << 1; }
    inline bool isTop(size_t index) { return index == 0; }
    void percolateUp()
    {
        size_t index = vec_.size() - 1;
        T save = std::move(vec_[index]);
        size_t p;
        while ((p = parent(index)) != index)
        {
            if (lessThanCmp_(vec_[p], save))
                break;
            vec_[index] = std::move(vec_[p]);
            index = p;
        }
        vec_[index] = std::move(save);
    }
    size_t properParent(T &save, size_t p, size_t lc, size_t rc)
    {
        assert(p < vec_.size());
        bool lcValid = lc < vec_.size();
        bool rcValid = rc < vec_.size();
        if (!lcValid && !rcValid)
        {
            return p;
        }
        else if (lcValid && !rcValid)
        {
            return lessThanCmp_(vec_[lc], save) ? lc : p;
        }
        else if (!lcValid && rcValid)
        {
            return lessThanCmp_(vec_[rc], save) ? rc : p;
        }
        else
        {
            size_t LRMax = lessThanCmp_(vec_[rc], vec_[lc]) ? rc : lc;
            return lessThanCmp_(vec_[LRMax], save) ? LRMax : p;
        }
    }
    void perolateDown()
    {
        size_t index = 0;
        T save = std::move(vec_[0]);
        size_t newParent;
        while (index != (newParent = properParent(save, index, lChild(index), rChild(index))))
        {
            vec_[index] = std::move(vec_[newParent]);
            index = newParent;
        }
        vec_[index] = std::move(save);
    }

public:
    BinaryHeap() = delete;
    explicit BinaryHeap(std::function<bool(T &, T &)> lessThanCmp)
        : vec_(), lessThanCmp_(lessThanCmp) {}
    BinaryHeap(BinaryHeap &&movedHeap)
        : vec_(std::move(movedHeap.vec_)), lessThanCmp_(movedHeap.lessThanCmp_) {}
    BinaryHeap(const BinaryHeap &heap2Copy)
        : vec_(heap2Copy.vec_), lessThanCmp_(heap2Copy.lessThanCmp_) {}
    inline T &top() { return vec_[0]; }
    T popTop()
    {
        assert(vec_.size() > 0 && "Trying to pop an empty heap");
        T res = std::move(vec_[0]);
        vec_[0] = std::move(vec_[vec_.size() - 1]);
        vec_.pop_back();
        if (vec_.size())
            perolateDown();
        return std::move(res);
    }
    inline size_t size() { return vec_.size(); }
    size_t insert(const T &item)
    {
        vec_.push_back(item);
        percolateUp();
        return size();
    }
    size_t insert(T &&item)
    {
        vec_.push_back(std::move(item));
        percolateUp();
        return size();
    }
};
