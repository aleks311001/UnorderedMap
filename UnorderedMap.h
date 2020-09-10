#ifndef UNORDEREDMAPTASK_UNORDERED_MAP_H
#define UNORDEREDMAPTASK_UNORDERED_MAP_H

#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include "ListUM.h"

///
///UnorderedMap
///

template<typename Key, typename Value, typename Hash = std::hash<Key>,
         typename Equal = std::equal_to<Key>, typename Alloc = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
public:
    typedef std::pair<const Key, Value> NodeType;

    explicit UnorderedMap(size_t numBuckets = 8);
    UnorderedMap(const UnorderedMap& other);
    UnorderedMap(UnorderedMap&& other) noexcept;
    ~UnorderedMap();
    UnorderedMap& operator=(const UnorderedMap& other);
    UnorderedMap& operator=(UnorderedMap&& other);

    template<bool is_const>
    class HelpIterator {
    private:
        typedef typename std::conditional<is_const, typename ListUM<NodeType, Alloc>::ConstIterator,
                                typename ListUM<NodeType, Alloc>::Iterator>::type ListIterator_;
        ListIterator_ iter_;
        explicit HelpIterator(ListIterator_ it) : iter_(it) {
        };
        friend class UnorderedMap;

    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef NodeType value_type;
        typedef int difference_type;
        typedef typename std::conditional<is_const, const NodeType*, NodeType*>::type pointer;
        typedef typename std::conditional<is_const, const NodeType&, NodeType&>::type reference;

        HelpIterator(const HelpIterator& other);
        HelpIterator& operator=(const HelpIterator& other);

        reference operator*() const;
        pointer operator->() const;

        HelpIterator& operator++();
        HelpIterator operator++(int);
        bool operator==(const HelpIterator& other) const;
        bool operator!=(const HelpIterator& other) const;
    };

    typedef HelpIterator<true> ConstIterator;
    typedef HelpIterator<false> Iterator;

    Iterator begin();
    Iterator end();
    ConstIterator begin() const;
    ConstIterator end() const;
    ConstIterator cbegin() const;
    ConstIterator cend() const;

    Value& operator[](const Key& key);
    const Value& at(const Key& key) const;
    Value& at(const Key& key);
    Iterator find(const Key& key);

    size_t size();
    void rehash(size_t count);
    void reserve(size_t count);
    size_t max_size() const;
    float max_load_factor() const;
    void max_load_factor(float ml);
    float load_factor() const;

    std::pair<Iterator, bool> insert(NodeType&& node);
    std::pair<Iterator, bool> insert(const NodeType& node);
    template<typename T>
    std::pair<Iterator, bool> insert(T&& node);
    template<typename It>
    void insert(const It& begin, const It& end);

    template<typename ...Args>
    std::pair<Iterator, bool> emplace(Args&& ... args);

    void erase(Iterator it);
    void erase(Iterator begin, Iterator end);

private:
    typedef typename ListUM<NodeType, Alloc>::Iterator ListIterator_;
    typedef typename ListUM<NodeType, Alloc>::Node ListNode_;
    typedef std::pair<ListIterator_, size_t> TypeBucket_;
    size_t numBuckets_;
    size_t size_;
    float maxLoadFactor_;
    ListUM<NodeType, Alloc> mainList_;
    typename Alloc::template rebind<TypeBucket_>::other bucketAlloc_;
    TypeBucket_* buckets_;
    Hash hash;
    Equal equal;

    template<typename T>
    auto insertHelp_(T&& node);
    void insertListNode_(ListNode_* node);

    void checkLoadFactor_();
    void swap_(UnorderedMap& other);
};



template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::UnorderedMap(size_t numBuckets)
        : numBuckets_(numBuckets),
          size_(0),
          maxLoadFactor_(0.75),
          mainList_(),
          bucketAlloc_(),
          buckets_(bucketAlloc_.allocate(numBuckets_)),
          hash(),
          equal() {
    for (size_t i = 0; i < numBuckets_; ++i) {
        bucketAlloc_.construct(buckets_ + i);
    }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::UnorderedMap(const UnorderedMap& other)
        : numBuckets_(other.numBuckets_),
          size_(other.size_),
          maxLoadFactor_(other.maxLoadFactor_),
          mainList_(other.mainList_),
          bucketAlloc_(other.bucketAlloc_),
          buckets_(bucketAlloc_.allocate(numBuckets_)),
          hash(other.hash),
          equal(other.equal) {
    for (size_t i = 0; i < numBuckets_; ++i) {
        bucketAlloc_.construct(buckets_ + i);
    }

    if (size_ > 0) {
        size_t prevHash = -1;
        size_t thisHash;
        for (auto it = mainList_.begin(); it != mainList_.end(); ++it) {
            thisHash = hash(it->first) % numBuckets_;

            if (thisHash != prevHash) {
                buckets_[thisHash].first = it;
                ++buckets_[thisHash].second;
                prevHash = thisHash;
            }
        }
    }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::UnorderedMap(UnorderedMap&& other) noexcept
        : numBuckets_(other.numBuckets_),
          size_(other.size_),
          maxLoadFactor_(other.maxLoadFactor_),
          mainList_(std::move(other.mainList_)),
          bucketAlloc_(std::move(other.bucketAlloc_)),
          buckets_(other.buckets_),
          hash(std::move(other.hash)),
          equal(std::move(other.equal)) {
    other.buckets_ = nullptr;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::~UnorderedMap() {
    if (buckets_ != nullptr) {
        for (size_t i = 0; i < numBuckets_; ++i) {
            bucketAlloc_.destroy(buckets_ + i);
        }
        bucketAlloc_.deallocate(buckets_, numBuckets_);
    }
    buckets_ = nullptr;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>&
UnorderedMap<Key, Value, Hash, Equal, Alloc>::operator=(const UnorderedMap& other) {
    UnorderedMap tmp = other;
    this->swap_(tmp);

    return *this;
}
template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>&
UnorderedMap<Key, Value, Hash, Equal, Alloc>::operator=(UnorderedMap&& other) {
    UnorderedMap tmp = std::move(other);
    this->swap_(tmp);

    return *this;
}

///-----
///Iterator
///-----

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<bool is_const>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::HelpIterator<is_const>::HelpIterator(
        const UnorderedMap::HelpIterator<is_const>& other)
        : iter_(other.iter_) {
}
template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<bool is_const>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::template HelpIterator<is_const>&
UnorderedMap<Key, Value, Hash, Equal, Alloc>::HelpIterator<is_const>::operator=(
        const UnorderedMap::HelpIterator<is_const>& other) {
    iter_ = other.iter_;
    return *this;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<bool is_const>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::template HelpIterator<is_const>::reference
UnorderedMap<Key, Value, Hash, Equal, Alloc>::HelpIterator<is_const>::operator*() const {
    return *iter_;
}
template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<bool is_const>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::template HelpIterator<is_const>::pointer
UnorderedMap<Key, Value, Hash, Equal, Alloc>::HelpIterator<is_const>::operator->() const {
    return &(*iter_);
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<bool is_const>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::template HelpIterator<is_const>&
UnorderedMap<Key, Value, Hash, Equal, Alloc>::HelpIterator<is_const>::operator++() {
    ++iter_;
    return *this;
}
template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<bool is_const>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::template HelpIterator<is_const>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::HelpIterator<is_const>::operator++(int) {
    auto copy = *this;
    ++*this;
    return copy;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<bool is_const>
bool UnorderedMap<Key, Value, Hash, Equal, Alloc>::template HelpIterator<is_const>::operator==(
        const UnorderedMap::HelpIterator<is_const>& other) const {
    return iter_ == other.iter_;
}
template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<bool is_const>
bool UnorderedMap<Key, Value, Hash, Equal, Alloc>::template HelpIterator<is_const>::operator!=(
        const UnorderedMap::HelpIterator<is_const>& other) const {
    return iter_ != other.iter_;
}

///-----
///Methods with Iterators
///-----

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::Iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::begin() {
    return Iterator(mainList_.begin());
}
template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::Iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::end() {
    return Iterator(mainList_.end());
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::ConstIterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::begin() const {
    return ConstIterator(mainList_.begin());
}
template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::ConstIterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::end() const {
    return ConstIterator(mainList_.end());
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::ConstIterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::cbegin() const {
    return ConstIterator(mainList_.begin());
}
template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::ConstIterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::cend() const {
    return ConstIterator(mainList_.end());
}

///-----
///lookup
///-----

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
Value& UnorderedMap<Key, Value, Hash, Equal, Alloc>::operator[](const Key& key) {
    try {
        return at(key);
    } catch (std::out_of_range& e) {
        return emplace(key, Value()).first->second;
    }
}
template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
const Value& UnorderedMap<Key, Value, Hash, Equal, Alloc>::at(const Key& key) const {
    return at(key);
}
template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
Value& UnorderedMap<Key, Value, Hash, Equal, Alloc>::at(const Key& key) {
    auto it = find(key);
    if (it != end()) {
        return it->second;
    }

    throw std::out_of_range("key not found");
}
template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::Iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::find(const Key& key) {
    size_t indexBucket = hash(key) % numBuckets_;

    size_t i = 0;
    for (auto it = buckets_[indexBucket].first; i < buckets_[indexBucket].second; ++it, ++i) {
        if (equal(it->first, key)) {
            return Iterator(it);
        }
    }

    return end();
}

///-----
///Capacity and hash
///-----
template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
size_t UnorderedMap<Key, Value, Hash, Equal, Alloc>::size() {
    return size_;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::rehash(size_t count) {
    if (count < size_ / maxLoadFactor_) {
        count = std::ceil(size_ / maxLoadFactor_);
    }

    UnorderedMap<Key, Value, Hash, Equal, Alloc> newUnorderedMap(count);
    newUnorderedMap.maxLoadFactor_ = maxLoadFactor_;

    for (auto it = mainList_.begin(); it != mainList_.end();) {
        newUnorderedMap.insertListNode_(mainList_.extractNode(it++));
    }

    *this = std::move(newUnorderedMap);
}
template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::reserve(size_t count) {
    rehash(std::ceil(count / max_load_factor()));
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
size_t UnorderedMap<Key, Value, Hash, Equal, Alloc>::max_size() const {
    return maxLoadFactor_ * numBuckets_;
}
template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
float UnorderedMap<Key, Value, Hash, Equal, Alloc>::max_load_factor() const {
    return maxLoadFactor_;
}
template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::max_load_factor(float ml) {
    maxLoadFactor_ = ml;
    checkLoadFactor_();
}
template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
float UnorderedMap<Key, Value, Hash, Equal, Alloc>::load_factor() const {
    return static_cast<float>(size_) / numBuckets_;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::checkLoadFactor_() {
    if (maxLoadFactor_ < load_factor()) {
        rehash(numBuckets_ * load_factor() / maxLoadFactor_ * 2);
    }
}

///-----
///Modifiers
///-----

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::insertListNode_(ListNode_* node) {
    checkLoadFactor_();

    size_t indexBucket = hash(node->key->first) % numBuckets_;
    if (buckets_[indexBucket].second > 0) {
        mainList_.insert_after(buckets_[indexBucket].first, node);
    } else {
        buckets_[indexBucket].first = mainList_.push_front(node);
    }

    ++buckets_[indexBucket].second;
    ++size_;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<typename T>
auto UnorderedMap<Key, Value, Hash, Equal, Alloc>::insertHelp_(T&& node) {
    checkLoadFactor_();

    auto it = find(node->key->first);
    if (it != end()) {
        mainList_.delNode(node);
        return std::pair<UnorderedMap::Iterator, bool>(it, false);
    }

    size_t indexBucket = hash(node->key->first) % numBuckets_;

    if (buckets_[indexBucket].second > 0) {
        it = Iterator(mainList_.insert_after(buckets_[indexBucket].first, node));
    } else {
        buckets_[indexBucket].first = mainList_.push_front(node);
        it = Iterator(buckets_[indexBucket].first);
    }

    ++buckets_[indexBucket].second;
    ++size_;

    return std::pair<Iterator, bool>(it, true);
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::Iterator, bool>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::insert(UnorderedMap::NodeType&& node) {
    return insertHelp_(mainList_.makeNode(std::move(node)));
}
template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::Iterator, bool>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::insert(const UnorderedMap::NodeType& node) {
    return insertHelp_(mainList_.makeNode(node));
}
template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<typename T>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::Iterator, bool>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::insert(T&& node) {
    return insertHelp_(mainList_.makeNode(std::forward<T>(node)));
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<typename It>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::insert(const It& begin, const It& end) {
    for (It it = begin; it != end; ++it) {
        insert(*it);
    }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<typename... Args>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::Iterator, bool>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::emplace(Args&& ... args) {
    return insertHelp_(mainList_.makeNode(std::forward<Args>(args)...));
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::erase(UnorderedMap::Iterator it) {
    size_t indexBucket = hash(it->first) % numBuckets_;
    if (buckets_[indexBucket].first == it.iter_) {
        ++buckets_[indexBucket].first;
    }
    mainList_.erase(it.iter_);
    --buckets_[indexBucket].second;
    --size_;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::erase(UnorderedMap::Iterator begin, UnorderedMap::Iterator end) {
    for (Iterator it = begin; it != end;) {
        erase(it++);
    }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::swap_(UnorderedMap& other) {
    std::swap(numBuckets_, other.numBuckets_);
    std::swap(size_, other.size_);
    std::swap(maxLoadFactor_, other.maxLoadFactor_);
    std::swap(mainList_, other.mainList_);
    std::swap(bucketAlloc_, other.bucketAlloc_);
    std::swap(buckets_, other.buckets_);
    std::swap(hash, other.hash);
    std::swap(equal, other.equal);
}

#endif //UNORDEREDMAPTASK_UNORDERED_MAP_H
