#include <iostream>
#include <vector>

template <size_t N>
class StackStorage {
private:
    char storage_[N];
    size_t shift = 0;

public:
    StackStorage() {}

    char* get_storage(size_t size, size_t alignment) {
        void* ins_point = reinterpret_cast<void*>(storage_ + shift);
        size_t length = N - shift;
        std::align(alignment, size, ins_point, length);
        shift = (reinterpret_cast<char*>(ins_point) - storage_) + size;
        return reinterpret_cast<char*>(ins_point);
    }

    ~StackStorage() {}
};

template <typename T, size_t N>
class StackAllocator {
private:
    StackStorage<N>* storage_;

public:
    using value_type = T;

    template <typename U>
    struct rebind {
        using other = StackAllocator<U, N>;
    };

    StackStorage<N>* Storage() const { return storage_; }

    StackAllocator() {}
    StackAllocator(StackStorage<N>& storage) : storage_(&storage) {}

    template <typename U>
    StackAllocator(const StackAllocator<U, N>& other)
        : storage_(other.Storage()) {}

    template <typename U>
    StackAllocator& operator=(StackAllocator<U, N>& other) {
        storage_ = other.storage_;
        return *this;
    }

    ~StackAllocator() {}

    T* allocate(size_t n) {
        return reinterpret_cast<T*>((*storage_).get_storage(sizeof(T) * n, alignof(T)));
    }

    void deallocate(T*, size_t) {}
};

template <typename T, typename Allocator = std::allocator<T>>
class List {
private:
    struct BaseNode {
        BaseNode* prev;
        BaseNode* next;
        BaseNode() : prev(nullptr), next(nullptr) {}
        BaseNode(BaseNode* ptr_to_prev, BaseNode* ptr_to_next)
            : prev(ptr_to_prev), next(ptr_to_next) {}
    };
    void tie_nods() {
        fake_start.next = &fake_end;
        fake_end.prev = &fake_start;
    }

    struct Node : BaseNode {
        T value;
        Node() {}
        Node(const T& v, BaseNode* ptr_to_prev, BaseNode* ptr_to_next)
            : BaseNode(ptr_to_prev, ptr_to_next), value(v) {}
        Node(const T& v) : BaseNode(nullptr, nullptr), value(v) {}
    };

    using NodeAlloc =
        typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    [[no_unique_address]] NodeAlloc object_alloc;

    size_t size_ = 0;
    BaseNode fake_start;
    BaseNode fake_end;

    template <typename... Args>
    void create_list(size_t n, const Args&... args);
    void create_new_node(const T& value, BaseNode* next_node);
    void delete_node(BaseNode* node);

    template <typename U>
    class common_iter {
    private:
        BaseNode* ptr_ = nullptr;

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using pointer = U*;
        using difference_type = ptrdiff_t;
        using reference = U&;
        using value_type = U;

        explicit common_iter(BaseNode* ptr) : ptr_(ptr) {}

        common_iter(const common_iter<T>& it2) : ptr_(it2.ptr()) {}

        common_iter& operator++() {
            ptr_ = ptr_->next;
            return *this;
        }

        common_iter operator++(int) {
            common_iter ans = *this;
            ptr_ = ptr_->next;
            return ans;
        }

        common_iter& operator--() {
            ptr_ = ptr_->prev;
            return *this;
        }

        common_iter operator--(int) {
            common_iter ans = *this;
            ptr_ = ptr_->prev;
            return ans;
        }

        U& operator*() { return reinterpret_cast<Node*>(ptr_)->value; }

        U* operator->() { return ptr_; }

        BaseNode* ptr() const { return ptr_; }

        template <typename U2>
        bool operator==(const common_iter<U2>& it2) const {
            return ptr_ == it2.ptr();
        }

        template <typename U2>
        bool operator!=(const common_iter<U2>& it2) const {
            return ptr_ != it2.ptr();
        }

        common_iter& operator=(const common_iter<T>& it2) {
            ptr_ = it2.ptr();
            return *this;
        }
    };

public:
    using iterator = common_iter<T>;
    using const_iterator = common_iter<const T>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    List();
    List(size_t n);
    List(size_t n, const T& value);
    List(Allocator& alloc);
    List(size_t n, Allocator& alloc);
    List(size_t n, const T& value, Allocator& alloc);

    Allocator get_allocator() { return object_alloc; };

    List(const List& list);
    ~List();
    List& operator=(const List& oth_list);

    size_t size() const;

    void push_back(const T& value);
    void push_front(const T& value);
    void pop_back();
    void pop_front();

    iterator begin() { return iterator(fake_start.next); }
    iterator end() { return iterator(&fake_end); }
    const_iterator begin() const { return const_iterator(fake_start.next); }
    const_iterator end() const {
        return const_iterator(const_cast<BaseNode*>(&fake_end));
    }
    const_iterator cbegin() const { return const_iterator(fake_start.next); }
    const_iterator cend() const {
        return const_iterator(const_cast<BaseNode*>(&fake_end));
    }
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }
    const_reverse_iterator rend() const {
        return const_reverse_iterator(begin());
    }
    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(cend());
    }
    const_reverse_iterator crend() const {
        return const_reverse_iterator(cbegin());
    }

    void insert(const_iterator it, const T& value);
    void erase(const_iterator it);
};

template <typename T, typename Allocator>
List<T, Allocator>::List() {
    tie_nods();
}

template <typename T, typename Allocator>
List<T, Allocator>::List(size_t n) {
    tie_nods();
    create_list(n);
}

template <typename T, typename Allocator>
List<T, Allocator>::List(size_t n, const T& value) {
    tie_nods();
    create_list(n, value);
}

template <typename T, typename Allocator>
List<T, Allocator>::List(Allocator& alloc) : object_alloc(alloc) {
    tie_nods();
}

template <typename T, typename Allocator>
List<T, Allocator>::List(size_t n, Allocator& alloc)
    : object_alloc(alloc) {
    tie_nods();
    create_list(n);
}

template <typename T, typename Allocator>
List<T, Allocator>::List(size_t n, const T& value, Allocator& alloc)
    : object_alloc(alloc) {
    tie_nods();
    create_list(n, value);
}

template <typename T, typename Allocator>
template <typename... Args>
void List<T, Allocator>::create_list(size_t n, const Args&... args) {
    for (size_t i = 0; i < n; ++i) {
        Node* new_node = nullptr;
        try {
            new_node =
                std::allocator_traits<NodeAlloc>::allocate(object_alloc, 1);
            std::allocator_traits<NodeAlloc>::construct(object_alloc, new_node,
                args...);
            fake_end.prev->next = new_node;
            new_node->prev = fake_end.prev;
            new_node->next = &fake_end;
            fake_end.prev = new_node;
            ++size_;
        }
        catch (...) {
            std::allocator_traits<NodeAlloc>::deallocate(object_alloc, reinterpret_cast<Node*>(new_node), 1);
            while (size_ > 0) {
                delete_node(fake_start.next);
            }
            throw;
        }
    }
}

template <typename T, typename Allocator>
List<T, Allocator>::List(const List& oth_list)
    : object_alloc(std::allocator_traits<NodeAlloc>::select_on_container_copy_construction(oth_list.object_alloc)) {
    tie_nods();
    const BaseNode* cur_node = &oth_list.fake_end;
    for (size_t i = 0; i < oth_list.size_; ++i) {
        cur_node = cur_node->prev;
        try {
            create_new_node(reinterpret_cast<const Node*>(cur_node)->value,
                fake_start.next);
        }
        catch (...) {
            while (size_ > 0) {
                delete_node(fake_start.next);
            }
        }
    }
}

template <typename T, typename Allocator>
void List<T, Allocator>::create_new_node(const T& value, BaseNode* next_node) {
    Node* new_node;
    try {
        new_node = std::allocator_traits<NodeAlloc>::allocate(object_alloc, 1);
        std::allocator_traits<NodeAlloc>::construct(object_alloc, new_node, value);
        next_node->prev->next = new_node;
        new_node->prev = next_node->prev;
        new_node->next = next_node;
        next_node->prev = new_node;
        ++size_;
    }
    catch (...) {
        std::allocator_traits<NodeAlloc >::deallocate(object_alloc, new_node, 1);
        throw;
    }
}

template <typename T, typename Allocator>
void List<T, Allocator>::delete_node(BaseNode* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    std::allocator_traits<NodeAlloc>::destroy(object_alloc,
        reinterpret_cast<Node*>(node));
    std::allocator_traits<NodeAlloc>::deallocate(
        object_alloc, reinterpret_cast<Node*>(node), 1);
    --size_;
}

template <typename T, typename Allocator>
List<T, Allocator>::~List() {
    while (size_ > 0) {
        delete_node(fake_end.prev);
    }
}

template <typename T, typename Allocator>
List<T, Allocator>& List<T, Allocator>::operator=(const List& oth_list) {
    List new_list;
    if (std::allocator_traits<NodeAlloc>::propagate_on_container_copy_assignment::value) {
        new_list.object_alloc = oth_list.object_alloc;
    }
    else {
        new_list.object_alloc = object_alloc;
    }
    const BaseNode* cur_node = &oth_list.fake_end;
    for (size_t i = 0; i < oth_list.size_; ++i) {
        cur_node = cur_node->prev;
        try {
            new_list.create_new_node(reinterpret_cast<const Node*>(cur_node)->value,
                new_list.fake_start.next);
        }
        catch (...) {
            while (new_list.size_ > 0) {
                new_list.delete_node(new_list.fake_start.next);
            }
        }
    }
    std::swap(fake_start, new_list.fake_start);
    std::swap(fake_end, new_list.fake_end);
    std::swap(fake_start.next->prev, new_list.fake_start.next->prev);
    std::swap(fake_end.prev->next, new_list.fake_end.prev->next);
    std::swap(size_, new_list.size_);
    std::swap(object_alloc, new_list.object_alloc);
    return *this;
}

template <typename T, typename Allocator>
size_t List<T, Allocator>::size() const {
    return size_;
}

template <typename T, typename Allocator>
void List<T, Allocator>::push_back(const T& value) {
    create_new_node(value, &fake_end);
}

template <typename T, typename Allocator>
void List<T, Allocator>::push_front(const T& value) {
    create_new_node(value, fake_start.next);
}

template <typename T, typename Allocator>
void List<T, Allocator>::pop_back() {
    delete_node(fake_end.prev);
}

template <typename T, typename Allocator>
void List<T, Allocator>::pop_front() {
    delete_node(fake_start.next);
}

template <typename T, typename Allocator>
void List<T, Allocator>::insert(const_iterator it, const T& value) {
    create_new_node(value, it.ptr());
}

template <typename T, typename Allocator>
void List<T, Allocator>::erase(const_iterator it) {
    delete_node(it.ptr());
}