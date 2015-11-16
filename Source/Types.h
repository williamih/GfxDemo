#ifndef TYPES_H
#define TYPES_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

template<class T, class Name>
struct PrimitiveWrap {
    typedef T Type;

    PrimitiveWrap() : value() {}
    explicit PrimitiveWrap(T v) : value(v) {}
    operator const T&() const { return value; }
    operator T&() { return value; }

    PrimitiveWrap& operator=(const PrimitiveWrap& rhs)
    { value = rhs.value; return *this; }
private:
    T value;
};

#define DECLARE_PRIMITIVE_WRAPPER(type, name) \
    struct Tag_##name; \
    typedef PrimitiveWrap<type, Tag_##name> name

#endif // TYPES_H
