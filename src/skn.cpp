#pragma once

#include <cmath>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>


typedef unsigned int uint;

#define KB(value) ((value) * 1024)
#define MB(value) (KB(value) * 1024)

// defer
template <typename F> struct privDefer {
    F f;
    privDefer(F f) : f(f) {}
    ~privDefer() noexcept { f(); }
};

template <typename F> privDefer<F> defer_func(F f) { return privDefer<F>(f); }

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x) DEFER_2(x, __COUNTER__)
#define defer(code) auto DEFER_3(_defer_) = defer_func([&]() { code; })

#define TODO(text) __builtin_debugtrap()

// Slice
template <typename T, bool zero = false> struct OldSlice {
    size_t len;
    T *ptr;

    OldSlice() : len(0), ptr(0) {}

    OldSlice(size_t len, T *ptr) : len(len), ptr(ptr) {}

    OldSlice<const char>(const char *text) : len(strlen(text)), ptr(text) {}

    OldSlice<const char>(const OldSlice<char> &slice) : len(slice.len), ptr(slice.ptr) {}

    T &operator[](size_t index) {
        SDL_assert(index < len);
        return ptr[index];
    }

    const T &operator[](size_t index) const {
        SDL_assert(index < len);
        return ptr[index];
    }

    bool operator==(OldSlice other) const {
        if (len != other.len) return false;
        for (size_t i = 0; i < len; i++) {
            if (ptr[i] != other[i]) return false;
        }
        return true;
    }

    T *begin() { return ptr; }
    T *end() { return ptr + len; }

    OldSlice<T, false> withoutZero() {
        static_assert(zero);
        return {len, ptr};
    }

    OldSlice<T, false> withZero() {
        static_assert(zero);
        return {len + 1, ptr};
    }

    void sort(int (*sortFn)(const void *a, const void *b), size_t offset = 0) {
        SDL_assert(offset <= len);
        qsort(ptr + offset, len - offset, sizeof(T), sortFn);
    }

    static OldSlice<const char> fromStrZ(const char *str) { return {strlen(str), str}; }
};

template <typename T> using SliceZ = OldSlice<T, true>;

static std::string_view getStem(const char *str_z) {
    ptrdiff_t len = strlen(str_z);
    SDL_assert(len > 0);
    for (ptrdiff_t i = len - 1; i >= 0; i--) {
        if (str_z[i] == '.') return {str_z, size_t(i)};
    }
    return {str_z, size_t(len)};
}

// Array
template <typename T, size_t N> struct OldArray {
    T data[N];

    constexpr T &operator[](size_t index) {
        SDL_assert(index < len());
        return data[index];
    }

    const T &operator[](size_t index) const {
        SDL_assert(index < len());
        return data[index];
    }

    constexpr size_t len() const {
        return N;
    }

    T *begin() { return data; }
    T *end() { return data + len(); }
};

#define ARRAY_LEN(array) (sizeof(array) / sizeof((array)[0]))

template <typename T> T *appendOld(T *data, size_t *len, size_t capacity, T value) {
    SDL_assert(*len < capacity);
    const size_t index = *len;
    data[(*len)++] = value;
    return &data[index];
}

template <typename T> void swapRemove(T *data, size_t *len, size_t index) {
    SDL_assert(index < *len);
    data[index] = data[--(*len)];
}

template <typename T> T swapPop(T *data, size_t *len, size_t index) {
    SDL_assert(index < *len);
    T removed = data[index];
    data[index] = data[--(*len)];
    return removed;
}

// Fixed
template <typename T> struct Fixed : public OldSlice<T> {
    size_t capacity;

    static Fixed create(OldSlice<T> slice) { return {{0, slice.ptr}, slice.len}; }

    void init(OldSlice<T> slice) {
        this->ptr = slice.ptr;
        this->len = 0;
        capacity = slice.len;
    }

    T *append(T value) { return ::appendOld(this->ptr, &this->len, capacity, value); }
    void swapRemove(size_t index) { ::swapRemove(this->ptr, &this->len, index); }
    T swapPop(size_t index) { return ::swapPop(this->ptr, &this->len, index); }
};

// FixedStack
template <typename T, size_t N> struct FixedStack : OldArray<T, N> {
    static_assert(N > 0);
    size_t len;
    size_t next;

    void push(T value) {
        this->data[next] = value;
        next = (next + 1) % N;
        if (len < N) len++;
    }

    T pop() {
        SDL_assert(len > 0);
        len--;
        next = (N + next - 1) % N;
        return this->data[next];
    }

    T peek() {
        SDL_assert(len > 0);
        auto index = (N + next - 1) % N;
        return this->data[index];
    }

    void reset() {
        len = 0;
        next = 0;
    }
};

// Arena
struct Arena {
    uint8_t *mem;
    size_t capacity;
    size_t next_position;
    FixedStack<size_t, 8> positions;

    static Arena init(size_t size) {
        Arena self = {};
        SDL_assert(size > 0);
        self.mem =
            (uint8_t *)mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        SDL_assert(self.mem != MAP_FAILED);
        self.capacity = size;
        return self;
    }

    void deinit() const { SDL_assert(munmap(mem, capacity) == 0); }

    void checkCapacity(size_t pos) const {
        if (pos > capacity) {
            auto kb = float(pos) / 1024.0F;
            auto mb = kb / 1024.0F;
            if (pos < 10000) {
                printf("You need more memory: %luB\n", pos);
            } else if (kb < 10000) {
                printf("You need more memory: %.02fK\n", kb);
            } else {
                printf("You need more memory: %.02fM\n", mb);
            }
            SDL_assert((pos) <= capacity);
        }
    }

    template <typename T> OldSlice<T> alloc(size_t len) {
        if (len == 0) return {};
        const auto size = len * sizeof(T);
        auto mod = next_position % alignof(T);
        const size_t pos = mod ? next_position + (alignof(T) - mod) : next_position;
        checkCapacity(pos + size);
        positions.push(pos);
        next_position = pos + size;
        return OldSlice<T>{len, (T *)(mem + pos)};
    };

    template <typename T> SliceZ<T> allocZ(size_t len) {
        auto slice = alloc<T>(len + 1);
        slice[len] = 0;
        return {len, slice.ptr};
    };

    template <typename T> void free(OldSlice<T> slice) {
        if (slice.len == 0) return;
        const size_t pos = checkAndGetPosition((uint8_t *)slice.ptr);
        if (positions.len > 0 and pos == positions.peek()) next_position = positions.pop();
    }

    template <typename T> void free(SliceZ<T> slice_z) { free(slice_z.withZero()); }

    template <typename T> OldSlice<T> realloc(OldSlice<T> slice, size_t new_len) {
        const auto new_size = new_len * sizeof(T);
        if (new_len == 0) {
            free(slice);
            return {};
        }
        if (slice.len == 0) return alloc<T>(new_len);
        const size_t pos = checkAndGetPosition((uint8_t *)slice.ptr);
        if (positions.len == 0 or pos != positions.peek()) {
            OldSlice<T> out_slice = alloc<T>(new_len);
            memcpy(out_slice.ptr, slice.ptr, std::min(slice.len * sizeof(T), new_size));
            return out_slice;
        }
        SDL_assert((pos % alignof(T)) == 0);
        checkCapacity(pos + new_size);
        next_position = pos + new_size;
        slice.len = new_len;
        return slice;
    }

    size_t checkAndGetPosition(const uint8_t *ptr) const {
        auto diff = ptrdiff_t(ptr) - ptrdiff_t(mem);
        SDL_assert(diff >= 0);
        auto pos = size_t(diff);
        SDL_assert(pos < capacity);
        return pos;
    }

    void reset() {
        positions.reset();
        next_position = 0;
    }

    // dupe z
    SliceZ<char> dupeZ(OldSlice<const char> src) {
        auto slice_z = allocZ<char>(src.len);
        memcpy(slice_z.ptr, src.ptr, src.len);
        return slice_z;
    }

    SliceZ<char> dupeZ(const char *src) {
        auto slice_z = allocZ<char>(strlen(src));
        strlcpy(slice_z.ptr, src, slice_z.len + 1);
        return slice_z;
    }

    SliceZ<char> dupeAndFreeZ(const char *src, void (*freeFn)(void *p) = ::free) {
        auto string = dupeZ(src);
        freeFn((void *)src);
        return string;
    }

    // dupe const
    OldSlice<const char> dupeConst(OldSlice<const char> src) {
        auto dst = alloc<char>(src.len);
        SDL_assert(dst.ptr != 0);
        memcpy(dst.ptr, src.ptr, src.len);
        return {dst.len, dst.ptr};
    }

    OldSlice<const char> dupeConst(const char *src) { return dupeConst({strlen(src), src}); }

    // dupe const z
    SliceZ<const char> dupeConstZ(const char *src) {
        auto slice_z = dupeZ(src);
        return {slice_z.len, slice_z.ptr};
    }

    SliceZ<char> vAllocPrintZ(const char *fmt, va_list ap) {
        va_list copy;

        va_copy(copy, ap);
        const auto len = vsnprintf(0, 0, fmt, copy);
        va_end(copy);

        SDL_assert(len >= 0);
        SliceZ<char> slice = allocZ<char>(len);

        SDL_assert(vsnprintf(slice.ptr, len + 1, fmt, ap) == len);

        return slice;
    }

    SliceZ<char> allocPrintZ(const char *fmt, ...) __attribute__((format(gnu_printf, 2, 3))) {
        va_list ap;
        va_start(ap, fmt);
        auto slice = vAllocPrintZ(fmt, ap);
        va_end(ap);
        return slice;
    }

    OldSlice<char> allocPrint(const char *fmt, ...) __attribute__((format(gnu_printf, 2, 3))) {
        va_list ap;
        va_start(ap, fmt);
        auto slice = vAllocPrintZ(fmt, ap);
        va_end(ap);
        return slice.withoutZero();
    }
};

struct ScopeArena {
    Arena *arena;
    Arena tmp;
    size_t prev_capacity;
    ScopeArena(Arena *arena) : arena(arena), tmp({}) {
        const size_t arena_half_free = (arena->capacity - arena->next_position) / 2;

        tmp.mem = arena->mem + arena->next_position + arena_half_free;
        tmp.capacity = arena_half_free;

        prev_capacity = arena->capacity;

        arena->capacity = arena->next_position + arena_half_free;
    }
    ~ScopeArena() { arena->capacity = prev_capacity; }
};

// Dynamic
template <typename T> struct Dynamic : public OldSlice<T> {
    size_t capacity;

    static Dynamic init(Arena *a, size_t initial_capacity) {
        auto items = a->alloc<T>(initial_capacity);
        return {{0, items.ptr}, items.len};
    }

    void deinit(Arena *a) { a->free({capacity, this->ptr}); }

    void append(Arena *a, T value) {
        if (this->len == capacity) {
            size_t new_capacity = capacity ? capacity * 2 : 1;
            auto items = a->realloc(OldSlice<T>{capacity, this->ptr}, new_capacity);
            capacity = items.len;
            this->ptr = items.ptr;
        }
        this->ptr[this->len++] = value;
    }

    void pop() {
        SDL_assert(this->len > 0);
        this->len--;
    }

    void clear() { this->len = 0; }
};

inline size_t fnv1aHash(OldSlice<const char> string) {
    constexpr size_t fnv_prime = 1099511628211ULL;
    constexpr size_t fnv_offset_basis = 14695981039346656037ULL;

    size_t hash = fnv_offset_basis;

    for (auto c : string) {
        hash = hash xor (size_t) c;
        hash = hash * fnv_prime;
    }

    return hash;
}

template <typename T> struct HashMap {
    struct Item {
        OldSlice<const char> key;
        T value;
    };

    OldSlice<Item> data;

    static HashMap<T> init(Arena *a, size_t len) { return {.data = a->alloc<Item>(len)}; }

    static bool putInSlice(OldSlice<Item> slice, OldSlice<const char> key, T value) {
        SDL_assert(key.len);
        if (slice.len == 0) return false;
        auto hash = fnv1aHash(key);

        for (size_t offset = 0; offset < slice.len; offset++) {
            auto index = (hash + offset) % slice.len;

            if (slice[index].key.len) {
                SDL_assert(slice[index].key != key);
            } else {
                slice[index] = {key, value};
                return true;
            }
        }

        return false;
    }

    Item *getItem(OldSlice<const char> key) {
        SDL_assert(key.len);
        auto hash = fnv1aHash(key);

        for (size_t offset = 0; offset < data.len; offset++) {
            auto index = (hash + offset) % data.len;

            if (!data[index].key.len) {
                return 0;
            }

            if (data[index].key == key) {
                return &data[index];
            }
        }
        return 0;
    }

    T get(OldSlice<const char> key) {
        auto item = getItem(key);
        SDL_assert(item);
        return item->value;
    }

    T get(const char *key) { return get({strlen(key), key}); }

    void put(Arena *a, OldSlice<const char> key, T value) {
        while (true) {
            if (putInSlice(data, key, value)) return;
            auto new_len = data.len ? data.len * 2 : 1;
            auto new_data = a->alloc<Item>(new_len);
            for (const auto &item : data) {
                if (item.key.len) SDL_assert(putInSlice(new_data, item.key, item.value));
            }
            a->free(data);
            data = new_data;
        }
    }

    bool contains(const char *key) { return getItem(key) != 0; }
};
