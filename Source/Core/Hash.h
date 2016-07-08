#ifndef CORE_HASH_H
#define CORE_HASH_H

#include <algorithm> // for std::swap
#include <stdlib.h> // for malloc
#include <string.h> // for memset
#include "Core/Types.h" // for u32

/******************************************************************************
*
*   Hash-table implementation based on Robin Hood hashing.
*
*   Robin Hood hashing is a variant of open addressing, designed to reduce most
*   of the downsides of standard open addressing hash-tables.
*
*   See web articles by Sebastian Sylvan and by Emmanuel Goossaert for a
*   discussion of this design (in particular, the use of the 'backward-shift'
*   technique to handle deletion).
*
*   Benefits of this class include:
*
*     - High load factors can be used without adverse performance effects.
*
*     - Elements are stored in a linear array, without linked lists. This
*       improves cache performance.
*
*     - The class is designed as an 'intrusive' data structure where the value
*       must store the key in some way. In practical situations, this is
*       actually an advantage as it is often useful for value types to store
*       their key anyway. Hence use of this hash-table class can reduce
*       duplication of data by using the key already stored with the value.
*
*   This differs from e.g. std::unordered_map, which is based around chaining
*   (each bucket is a linked-list) and so has less ideal cache performance
*   characteristics.
*
*******************************************************************************/

template<class Key, class Value>
class THash {
public:
    THash();
    ~THash();

    u32 Count() const;

    template<class Func>
    bool Contains(const Key& key, const Func& getKey) const;

    // Returns a pointer to the value if it exists; otherwise nullptr.
    // Note that any modification made to the hashtable invalidates the returned
    // pointer.
    template<class Func>
    const Value* Get(const Key& key, const Func& getKey) const;

    template<class Func>
    void Insert(const Value& value, const Func& getKey);

    // Returns true iff the key was actually in the table (and therefore
    // deleted)
    template<class Func>
    bool Delete(const Key& key, const Func& getKey);

private:
    // Disallow copy construction/assignment
    THash(const THash&);
    THash& operator=(const THash&);

    static u32 HashKey(const Key& key);
    int ProbeDistance(u32 hash, int slotIndex, u32 mask) const;
    void Construct(int index, u32 hash, const Value& val);
    // Intentional pass-by-value of >value< as we need a copy of this
    // in the function.
    void InsertHelper(u32 hash, Value value);
    void Grow();

    template<class Func>
    int LookupIndex(const Key& key, const Func& getKey) const;

    // N.B. Capacity must be a power of two.
    static const u32 MIN_CAPACITY = 16;

    static const u32 MAX_LOAD_FACTOR_PERCENT = 90;

    u32 m_count;
    u32 m_capacity;
    Value* m_values;
    u32* m_hashes;
};

template<class Key, class Value>
THash<Key, Value>::THash()
    : m_count(0)
    , m_capacity(0)
    , m_values(NULL)
    , m_hashes(NULL)
{}

template<class Key, class Value>
THash<Key, Value>::~THash()
{
    for (u32 i = 0; i < m_count; ++i)
        if (m_hashes[i] != 0)
            m_values[i].~Value();
    free(m_values);
    delete[] m_hashes;
}

template<class Key, class Value>
u32 THash<Key, Value>::HashKey(const Key& key)
{
    u32 h = key.GetHashValue();
    // Ensure that 0 is never returned as a hash, as we use 0 to indicate an
    // empty slot.
    h |= (h == 0);
    return h;
}

template<class Key, class Value>
int THash<Key, Value>::ProbeDistance(u32 hash, int slotIndex, u32 mask) const
{
    const int desiredPos = hash & mask;
    return (slotIndex + m_capacity - desiredPos) & mask;
}

template<class Key, class Value>
void THash<Key, Value>::Construct(int index, u32 hash, const Value& val)
{
    new (&m_values[index]) Value(val);
    m_hashes[index] = hash;
}

template<class Key, class Value>
void THash<Key, Value>::InsertHelper(u32 hash, Value value)
{
    const u32 mask = m_capacity - 1;
    int pos = hash & mask;
    int dist = 0;
    for (;;) {
        if (m_hashes[pos] == 0) {
            Construct(pos, hash, value);
            return;
        }

        const int existingElementProbeDist = ProbeDistance(m_hashes[pos], pos,
                                                           mask);
        if (existingElementProbeDist < dist) {
            using std::swap;
            swap(hash, m_hashes[pos]);
            swap(value, m_values[pos]);
            dist = existingElementProbeDist;
        }

        pos = (pos + 1) & mask;
        ++dist;
    }
}

template<class Key, class Value>
void THash<Key, Value>::Grow()
{
    Value* const oldValues = m_values;
    u32* const oldHashes = m_hashes;
    const u32 oldCapacity = m_capacity;

    m_capacity *= 2;
    if (m_capacity == 0)
        m_capacity = MIN_CAPACITY;

    // Allocate new tables
    m_values = (Value*)malloc(m_capacity * sizeof(Value));
    m_hashes = new u32[m_capacity];
    // Flag all elements as free
    memset(m_hashes, 0, m_capacity * sizeof(u32));

    // Rehash old elements into the new tables
    for (u32 i = 0; i < oldCapacity; ++i) {
        const Value& val = oldValues[i];
        const u32 hash = oldHashes[i];
        if (hash != 0) {
            InsertHelper(hash, val);
            val.~Value();
        }
    }

    // Free old tables
    free(oldValues);
    delete[] oldHashes;
}

template<class Key, class Value>
template<class Func>
int THash<Key, Value>::LookupIndex(const Key& key, const Func& getKey) const
{
    if (m_capacity == 0)
        return -1;

    const u32 hash = HashKey(key);
    const u32 mask = m_capacity - 1;
    int pos = hash & mask;
    int dist = 0;
    for (;;) {
        if (m_hashes[pos] == 0)
            return -1;
        else if (dist > ProbeDistance(m_hashes[pos], pos, mask))
            return -1;
        else if (m_hashes[pos] == hash && getKey(m_values[pos]) == key)
            return pos;

        pos = (pos + 1) & mask;
        ++dist;
    }
}

template<class Key, class Value>
u32 THash<Key, Value>::Count() const
{
    return m_count;
}

template<class Key, class Value>
template<class Func>
bool THash<Key, Value>::Contains(const Key& key, const Func& getKey) const
{
    return LookupIndex(key, getKey) != -1;
}

template<class Key, class Value>
template<class Func>
const Value* THash<Key, Value>::Get(const Key& key, const Func& getKey) const
{
    const int index = LookupIndex(key, getKey);
    if (index == -1)
        return NULL;
    return &m_values[index];
}

template<class Key, class Value>
template<class Func>
void THash<Key, Value>::Insert(const Value& value, const Func& getKey)
{
    const int index = LookupIndex(getKey(value), getKey);
    if (index != -1) {
        m_values[index] = value;
    } else {
        ++m_count;
        const u32 resizeThreshold = (MAX_LOAD_FACTOR_PERCENT*m_capacity) / 100;
        if (m_count > resizeThreshold)
            Grow();
        InsertHelper(HashKey(getKey(value)), value);
    }
}

template<class Key, class Value>
template<class Func>
bool THash<Key, Value>::Delete(const Key& key, const Func& getKey)
{
    // Overall idea: use 'backward-shift' technique to move elements so that the
    // resulting empty slot ends up in a location where it won't cause issues
    // for the insert and find algorithms.

    const u32 mask = m_capacity - 1;
    int pos = LookupIndex(key, getKey);

    if (pos == -1)
        return false;

    for (;;) {
        const int nextPos = (pos + 1) & mask;

        // The 'stop bucket' is either an empty bucket (hash == 0), or one with
        // a distance-to-initial-bucket of 0 (i.e. the element is in the exact
        // location it hashed to).
        if ((m_hashes[nextPos] == 0)
            || ((m_hashes[nextPos] & mask) == nextPos))
            break;

        m_values[pos] = m_values[nextPos];
        m_hashes[pos] = m_hashes[nextPos];
        pos = nextPos;
    }

    // After shifting backwards, mark the no-longer-used slot as empty, and
    // call its destructor.
    m_hashes[pos] = 0;
    m_values[pos].~Value();

    --m_count;

    return true;
}

#endif // CORE_HASH_H
