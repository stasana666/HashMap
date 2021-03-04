#include <initializer_list>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>

template<class T>
class optional {
private:
    alignas(T) char data[sizeof(T)];
    bool flag;

public:
    optional()
            : flag(false) {
    }

    optional(const optional& other)
            : flag(other.flag) {
        if (flag) {
            new (data) T(*reinterpret_cast<const T*>(other.data));
        }
    }

    ~optional() {
        if (flag) {
            reinterpret_cast<T*>(data)->~T();
        }
    }

    optional& operator =(const optional& other) {
        if (this == &other) {
            return *this;
        }
        if (flag) {
            reset();
        }
        if (other.flag) {
            new (data) T(*reinterpret_cast<const T*>(other.data));
            flag = true;
        }
        return *this;
    }

    T& value() {
        return *reinterpret_cast<T*>(data);
    }

    const T& value() const {
        return *reinterpret_cast<const T*>(data);
    }

    bool has_value() const {
        return flag;
    }

    void insert(T x) {
        if (flag) {
            reset();
        }
        new (data) T(x);
        flag = true;
    }

    void reset() {
        if (flag) {
            reinterpret_cast<T*>(data)->~T();
        }
        flag = false;
    }

    T& operator *() {
        return *reinterpret_cast<T*>(data);
    }

    const T& operator *() const {
        return *reinterpret_cast<const T*>(data);
    }

    T* operator ->() {
        return reinterpret_cast<T*>(data);
    }

    const T* operator ->() const {
        return reinterpret_cast<const T*>(data);
    }
};

template<class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
public:
    HashMap(Hash hasher = Hash())
        : data(init_size())
        , current_size(0)
        , hasher(std::move(hasher)) {
    }

    template<class T>
    HashMap(T begin, T end, Hash hasher = Hash())
        : HashMap(hasher) {
        for (auto it = begin; it != end; ++it) {
            insert(*it);
        }
    }

    HashMap(const std::initializer_list<std::pair<KeyType, ValueType>>& list, Hash hasher = Hash())
        : HashMap(hasher) {
        for (const auto val : list) {
            insert(val);
        }
    }

    size_t size() const {
        return current_size;
    }

    bool empty() const {
        return !current_size;
    }

    Hash hash_function() const {
        return hasher;
    }

    void insert(const std::pair<KeyType, ValueType>& x) {
        if (need_reallocate()) {
            reallocate();
        }
        int ptr = hasher(x.first) % data.size();
        while (data[ptr].has_value()) {
            if (x.first == data[ptr]->first) {
                return;
            }
            ++ptr;
            if (ptr == (int)data.size()) {
                ptr = 0;
            }
        }
        data[ptr].insert(x);
        ++current_size;
    }

    void erase(const KeyType& key) {
        int ptr = hasher(key) % data.size();
        while (data[ptr].has_value()) {
            if (key == data[ptr]->first) {
                break;
            }
            ++ptr;
            if (ptr == (int)data.size()) {
                ptr = 0;
            }
        }
        if (!data[ptr].has_value()) {
            return;
        }
        if (key != data[ptr]->first) {
            return;
        }
        data[ptr].reset();
        --current_size;
        ++ptr;
        if (ptr == (int)data.size()) {
            ptr = 0;
        }
        std::vector<std::pair<KeyType, ValueType>> arr;
        while (data[ptr].has_value()) {
            arr.emplace_back(*data[ptr]);
            data[ptr].reset();
            --current_size;
            ++ptr;
            if (ptr == (int)data.size()) {
                ptr = 0;
            }
        }
        for (auto& x : arr) {
            insert(x);
        }
    }

    class iterator {
    public:

        iterator() {
        }

        std::pair<const KeyType, ValueType>& operator *() {
            return **it;
        }

        std::pair<const KeyType, ValueType>* operator ->() {
            return (*it).operator ->();
        }

        iterator& operator ++() {
            ++it;
            find();
            return *this;
        }

        iterator operator ++(int) {
            auto old = *this;
            ++it;
            find();
            return old;
        }

        bool operator ==(iterator other) {
            return it == other.it;
        }

        bool operator !=(iterator other) {
            return it != other.it;
        }

    private:
        iterator(typename std::vector<optional<std::pair<const KeyType, ValueType>>>::iterator it,
                 typename std::vector<optional<std::pair<const KeyType, ValueType>>>::iterator end)
            : it(it)
            , end(end) {
            find();
        }

        inline void find() {
            while (it != end && !it->has_value()) {
                ++it;
            }
        }

        typename std::vector<optional<std::pair<const KeyType, ValueType>>>::iterator it;
        typename std::vector<optional<std::pair<const KeyType, ValueType>>>::iterator end;

        friend class HashMap;
    };

    class const_iterator {
    public:

        const_iterator() {
        }

        const std::pair<const KeyType, ValueType>& operator *() {
            return **it;
        }

        const std::pair<const KeyType, ValueType>* operator ->() {
            return (*it).operator ->();
        }

        const_iterator& operator ++() {
            ++it;
            find();
            return *this;
        }

        const_iterator operator ++(int) {
            auto old = *this;
            ++it;
            find();
            return old;
        }

        bool operator ==(const_iterator other) {
            return it == other.it;
        }

        bool operator !=(const_iterator other) {
            return it != other.it;
        }

    private:
        const_iterator(typename std::vector<optional<std::pair<const KeyType, ValueType>>>::const_iterator it,
                       typename std::vector<optional<std::pair<const KeyType, ValueType>>>::const_iterator end)
            : it(it)
            , end(end) {
            find();
        }

        inline void find() {
            while (it != end && !it->has_value()) {
                ++it;
            }
        }

        typename std::vector<optional<std::pair<const KeyType, ValueType>>>::const_iterator it;
        typename std::vector<optional<std::pair<const KeyType, ValueType>>>::const_iterator end;

        friend class HashMap;
    };

    iterator begin() {
        return iterator(data.begin(), data.end());
    }

    iterator end() {
        return iterator(data.end(), data.end());
    }

    const_iterator begin() const {
        return const_iterator(data.cbegin(), data.cend());
    }

    const_iterator end() const {
        return const_iterator(data.cend(), data.cend());
    }

    const_iterator find(const KeyType& key) const {
        int ptr = hasher(key) % data.size();
        while (data[ptr].has_value()) {
            if (key == data[ptr]->first) {
                return const_iterator(data.begin() + ptr, data.end());
            }
            ++ptr;
            if (ptr == (int)data.size()) {
                ptr = 0;
            }
        }
        return const_iterator(data.end(), data.end());
    }

    iterator find(const KeyType& key) {
        int ptr = hasher(key) % data.size();
        while (data[ptr].has_value()) {
            if (key == data[ptr]->first) {
                return iterator(data.begin() + ptr, data.end());
            }
            ++ptr;
            if (ptr == (int)data.size()) {
                ptr = 0;
            }
        }
        return iterator(data.end(), data.end());
    }

    ValueType& operator [](const KeyType& key) {
        if (need_reallocate()) {
            reallocate();
        }
        int ptr = hasher(key) % data.size();
        while (data[ptr].has_value()) {
            if (data[ptr]->first == key) {
                return data[ptr]->second;
            }
            ++ptr;
            if (ptr == (int)data.size()) {
                ptr = 0;
            }
        }
        data[ptr].insert(std::make_pair<const KeyType, ValueType>(KeyType(key), ValueType()));
        ++current_size;
        return data[ptr]->second;
    }

    const ValueType& at(const KeyType& key) const {
        int ptr = hasher(key) % data.size();
        while (data[ptr].has_value()) {
            if (data[ptr]->first == key) {
                return data[ptr]->second;
            }
            ++ptr;
            if (ptr == (int)data.size()) {
                ptr = 0;
            }
        }
        throw std::out_of_range("");
    }

    void clear() {
        data.clear();
        data.resize(init_size());
        current_size = 0;
    }

private:

    inline void reallocate() {
        std::vector<optional<std::pair<const KeyType, ValueType>>> arr(data.size() * 2);
        data.swap(arr);
        current_size = 0;
        for (const auto& i : arr) {
            if (i.has_value()) {
                insert(*i);
            }
        }
    }

    inline bool need_reallocate() {
        return 2 * current_size > data.size();
    }

    inline static size_t init_size() {
        return 13;
    }

    std::vector<optional<std::pair<const KeyType, ValueType>>> data;
    size_t current_size;
    Hash hasher;
};
