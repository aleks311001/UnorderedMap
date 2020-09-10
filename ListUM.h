#ifndef UNORDEREDMAPTASK__LISTUM_H_
#define UNORDEREDMAPTASK__LISTUM_H_

#include <vector>
#include <string>
#include <iostream>

template<typename T, typename Alloc = std::allocator<T>>
class ListUM {
public:
    ListUM();
    ListUM(const ListUM& other);
    ListUM(ListUM&& other) noexcept;
    ~ListUM();
    ListUM& operator=(const ListUM& other);
    ListUM& operator=(ListUM&& other) noexcept;

    struct Node {
        T* key;
        Node* next;
        Node* prev;

        static std::allocator_traits<Alloc> allocTraits;
        static Alloc allocT_;

        template<typename ...Args>
        explicit Node(Args&& ...args);
        ~Node();
    };

    template<bool is_const>
    class HelpIterator {
    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef T value_type;
        typedef int difference_type;
        typedef typename std::conditional<is_const, const T*, T*>::type pointer;
        typedef typename std::conditional<is_const, const T&, T&>::type reference;

        HelpIterator() : now(nullptr) {
        };
        HelpIterator(const HelpIterator& other);
        HelpIterator& operator=(const HelpIterator& other);

        reference operator*() const;
        pointer operator->() const;

        HelpIterator& operator++();
        HelpIterator operator++(int);
        bool operator==(const HelpIterator& other) const;
        bool operator!=(const HelpIterator& other) const;

        friend class ListUM;
    private:
        explicit HelpIterator(Node* node) : now(node) {
        };
        Node* now;
    };

    typedef HelpIterator<true> ConstIterator;
    typedef HelpIterator<false> Iterator;

    Iterator begin();
    Iterator end();
    ConstIterator begin() const;
    ConstIterator end() const;
    ConstIterator cbegin() const;
    ConstIterator cend() const;

    Iterator insert_after(Iterator it, Node* node);
    template<typename ...Args>
    Iterator emplace_after(Iterator it, Args&& ...args);

    Iterator push_front(Node* node);
    Iterator push_back(Node* node);

    template<typename ...Args>
    Node* makeNode(Args&& ...args);
    void delNode(Node* node);

    void erase(Iterator it);
    Node* extractNode(Iterator it);

private:
    Node* first_;
    Node* last_;

    typename Alloc::template rebind<Node>::other alloc_;

    void connect_(Node* left, Node* right);
    void swap_(ListUM& other);
};

template<typename T, typename Alloc>
Alloc ListUM<T, Alloc>::Node::allocT_ = Alloc();
template<typename T, typename Alloc>
std::allocator_traits<Alloc> ListUM<T, Alloc>::Node::allocTraits = std::allocator_traits<Alloc>();



template<typename T, typename Alloc>
ListUM<T, Alloc>::ListUM(): first_(nullptr), last_(nullptr), alloc_() {
}

template<typename T, typename Alloc>
ListUM<T, Alloc>::ListUM(const ListUM& other): first_(nullptr), last_(nullptr), alloc_(other.alloc_) {
    Node* prev = nullptr;
    for (Node* node = other.first_; node != nullptr; node = node->next) {
        Node* new_node = makeNode(*(node->key));
        new_node->prev = prev;

        if (prev == nullptr) {
            first_ = new_node;
        } else {
            prev->next = new_node;
        }
        prev = new_node;
    }
    last_ = prev;
}

template<typename T, typename Alloc>
ListUM<T, Alloc>::ListUM(ListUM&& other) noexcept
        : first_(other.first_),
          last_(other.last_),
          alloc_(std::move(other.alloc_)) {
    other.first_ = nullptr;
    other.last_ = nullptr;
}

template<typename T, typename Alloc>
ListUM<T, Alloc>::~ListUM() {
    for (Node* node = first_, * next_node; node != nullptr; node = next_node) {
        next_node = node->next;
        alloc_.destroy(node);
        alloc_.deallocate(node, 1);
    }
    first_ = nullptr;
    last_ = nullptr;
}

template<typename T, typename Alloc>
ListUM<T, Alloc>& ListUM<T, Alloc>::operator=(const ListUM& other) {
    ListUM tmp = other;
    this->swap_(tmp);

    return *this;
}

template<typename T, typename Alloc>
ListUM<T, Alloc>& ListUM<T, Alloc>::operator=(ListUM&& other) noexcept {
    ListUM tmp = std::move(other);
    this->swap_(tmp);

    return *this;
}

///-----
///Iterators
///-----

template<typename T, typename Alloc>
template<bool is_const>
ListUM<T, Alloc>::HelpIterator<is_const>::HelpIterator(
        const ListUM::HelpIterator<is_const>& other): now(other.now) {
}
template<typename T, typename Alloc>
template<bool is_const>
typename ListUM<T, Alloc>::template HelpIterator<is_const>&
ListUM<T, Alloc>::HelpIterator<is_const>::operator=(const ListUM::HelpIterator<is_const>& other) {
    now = other.now;
    return *this;
}

template<typename T, typename Alloc>
template<bool is_const>
typename ListUM<T, Alloc>::template HelpIterator<is_const>::reference
ListUM<T, Alloc>::HelpIterator<is_const>::operator*() const {
    return *(now->key);
}
template<typename T, typename Alloc>
template<bool is_const>
typename ListUM<T, Alloc>::template HelpIterator<is_const>::pointer
ListUM<T, Alloc>::HelpIterator<is_const>::operator->() const {
    return now->key;
}

template<typename T, typename Alloc>
template<bool is_const>
typename ListUM<T, Alloc>::template HelpIterator<is_const>& ListUM<T, Alloc>::HelpIterator<is_const>::operator++() {
    now = now->next;
    return *this;
}
template<typename T, typename Alloc>
template<bool is_const>
typename ListUM<T, Alloc>::template HelpIterator<is_const> ListUM<T, Alloc>::HelpIterator<is_const>::operator++(int) {
    auto copy = *this;
    ++*this;
    return copy;
}

template<typename T, typename Alloc>
template<bool is_const>
bool ListUM<T, Alloc>::HelpIterator<is_const>::operator==(const ListUM::HelpIterator<is_const>& other) const {
    return now == other.now;
}
template<typename T, typename Alloc>
template<bool is_const>
bool ListUM<T, Alloc>::HelpIterator<is_const>::operator!=(const ListUM::HelpIterator<is_const>& other) const {
    return now != other.now;
}

///-----
///Node
///-----

template<typename T, typename Alloc>
template<typename... Args>
ListUM<T, Alloc>::Node::Node(Args&& ... args): key(nullptr), next(nullptr), prev(nullptr) {
    key = allocTraits.allocate(allocT_, 1);
    allocTraits.construct(allocT_, key, std::forward<Args>(args)...);
}

template<typename T, typename Alloc>
ListUM<T, Alloc>::Node::~Node() {
    allocTraits.destroy(allocT_, key);
    allocTraits.deallocate(allocT_, key, 1);
    key = nullptr;
    next = nullptr;
    prev = nullptr;
}

///-----
///Other methods
///-----

template<typename T, typename Alloc>
typename ListUM<T, Alloc>::Iterator ListUM<T, Alloc>::begin() {
    return Iterator(first_);
}
template<typename T, typename Alloc>
typename ListUM<T, Alloc>::Iterator ListUM<T, Alloc>::end() {
    return Iterator(nullptr);
}

template<typename T, typename Alloc>
typename ListUM<T, Alloc>::ConstIterator ListUM<T, Alloc>::begin() const {
    return ConstIterator(first_);
}
template<typename T, typename Alloc>
typename ListUM<T, Alloc>::ConstIterator ListUM<T, Alloc>::end() const {
    return ConstIterator(nullptr);
}
template<typename T, typename Alloc>
typename ListUM<T, Alloc>::ConstIterator ListUM<T, Alloc>::cbegin() const {
    return ConstIterator(first_);
}
template<typename T, typename Alloc>
typename ListUM<T, Alloc>::ConstIterator ListUM<T, Alloc>::cend() const {
    return ConstIterator(nullptr);
}

template<typename T, typename Alloc>
typename ListUM<T, Alloc>::Iterator ListUM<T, Alloc>::insert_after(ListUM::Iterator it, ListUM::Node* node) {
    if (it.now == nullptr && last_ == nullptr) {
        first_ = node;
        last_ = node;
    } else {
        connect_(node, it.now->next);
        connect_(it.now, node);
        if (it.now == last_) {
            last_ = node;
        }
    }

    return ListUM::Iterator(node);
}
template<typename T, typename Alloc>
template<typename... Args>
typename ListUM<T, Alloc>::Iterator ListUM<T, Alloc>::emplace_after(ListUM::Iterator it, Args&& ...args) {
    return insert_after(it, makeNode(std::forward<Args>(args)...));
}

template<typename T, typename Alloc>
typename ListUM<T, Alloc>::Iterator ListUM<T, Alloc>::push_front(ListUM::Node* node) {
    connect_(node, first_);
    first_ = node;
    if (last_ == nullptr) {
        last_ = first_;
    }

    return ListUM::Iterator(node);
}
template<typename T, typename Alloc>
typename ListUM<T, Alloc>::Iterator ListUM<T, Alloc>::push_back(ListUM::Node* node) {
    connect_(last_, node);
    last_ = node;
    if (first_ == nullptr) {
        first_ = last_;
    }

    return ListUM::Iterator(node);
}

template<typename T, typename Alloc>
void ListUM<T, Alloc>::erase(ListUM::Iterator it) {
    Node* node = extractNode(it);
    alloc_.destroy(node);
    alloc_.deallocate(node, 1);
}
template<typename T, typename Alloc>
typename ListUM<T, Alloc>::Node* ListUM<T, Alloc>::extractNode(ListUM::Iterator it) {
    connect_(it.now->prev, it.now->next);
    if (it.now == first_) {
        first_ = it.now->next;
    }
    if (it.now == last_) {
        last_ = it.now->prev;
    }
    it.now->next = nullptr;
    it.now->prev = nullptr;

    return it.now;
}

template<typename T, typename Alloc>
void ListUM<T, Alloc>::connect_(ListUM::Node* left, ListUM::Node* right) {
    if (left != nullptr) {
        left->next = right;
    }
    if (right != nullptr) {
        right->prev = left;
    }
}
template<typename T, typename Alloc>
template<typename... Args>
typename ListUM<T, Alloc>::Node* ListUM<T, Alloc>::makeNode(Args&& ... args) {
    auto* node = alloc_.allocate(1);
    alloc_.construct(node, std::forward<Args>(args)...);

    return node;
}

template<typename T, typename Alloc>
void ListUM<T, Alloc>::delNode(ListUM::Node* node) {
    alloc_.destroy(node);
    alloc_.deallocate(node, 1);
}

template<typename T, typename Alloc>
void ListUM<T, Alloc>::swap_(ListUM& other) {
    std::swap(first_, other.first_);
    std::swap(last_, other.last_);
    std::swap(alloc_, other.alloc_);
}


#endif //UNORDEREDMAPTASK__LISTUM_H_
