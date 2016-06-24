/******************************************************************************
 *
 *   IDLookupTable.h
 *
 ***/

/******************************************************************************
 *
 *   WHAT IT IS
 *
 *   This file defines the IDLookupTable class template, which is a
 *   container/collection class designed to store plain-old-data (POD) objects
 *   accessed by an integer ID.
 *
 *   Conceptually, it is similar to a std::map<ID, T>, where ID is some
 *   integer type, and T is a POD type. However, this class offers several
 *   advantages over a conventional map class:
 *
 *       1. It takes O(1) time to lookup or remove an object from the table,
 *           rather than O(log n) for std::map.
 *
 *       2. Amortized O(1) time to add an item to the table
 *          (amortized because the internal array may need to be resized).
 *
 *       3. Objects are stored contiguously in memory. The class does NOT
 *          perform a new memory allocation for every single object added.
 *
 *       4. This class does not use hashing, so there are no issues related
 *          to hash collisions, etc.
 *
 *
 *   TEMPLATE PARAMETERS
 *
 *   T - the POD struct to store
 *
 *   ID - integral data type used as the ID to access objects
 *
 *   NumIndexBits - number of bits in the ID that will be used to store an
 *                  index into the array. This determines the maximum number
 *                  of objects that can exist at any one time.
 *
 *   NumInnerIDBits - number of bits in the ID that will be used to store a
 *                    unique 'inner' ID associated with each object. This
 *                    inner ID is unique across all objects that have ever
 *                    been stored within the lookup table, both existing
 *                    objects and destroyed objects. This parameter
 *                    determines the maximum number of objects that can be
 *                    created (and potentially destroyed) across the
 *                    lifetime of the application.
 *
 *   HOW TO USE IT
 *
 *   Decide how many bits will be used to store the index. The maximum number
 *   of objects that can exist in the table at any one time is 2^(NumIndexBits).
 *
 *   Decide how many bits will be used to store the inner ID.
 *
 *
 *   NOTES
 *
 *   Restrictions:
 *       NumIndexBits + NumInnerIDBits must not exceed the number of bits
 *       in the ID template parameter. However, NumIndexBits + NumInnerIDBits
 *       may be less than the number of bits in the ID parameter, in which case
 *       some bits in the ID will be 'spare' (will not be used by this class),
 *       and so may be utilized by other parts of the application.
 *
 *   Thanks to:
 *       The BitSquid engine team for discussing the idea behind this data
 *       structure in a blog post at
 *http://bitsquid.blogspot.com/2011/09/managing-decoupling-part-4-id-lookup.html
 *
 ***/

#ifndef CORE_IDLOOKUPTABLE_H
#define CORE_IDLOOKUPTABLE_H

#include <vector>
#include <limits.h>
#include "Core/Macros.h"

template<class T, class ID, size_t NumIndexBits, size_t NumInnerIDBits>
class IDLookupTable {
public:
    STATIC_ASSERT(
        (NumIndexBits + NumInnerIDBits) <= (sizeof(ID) * 8),
        "NumIndexBits + NumInnerIDBits exceeds the bit width of the ID type"
    );

    // Note: the index is stored in the lower NumIndexBits bits and the inner ID
    // is stored in the higher NumInnerIDBits bits.
    static const ID INDEX_MASK = (ID(1) << NumIndexBits) - 1;
    static const ID INNER_ID_MASK = (ID(1) << NumInnerIDBits) - 1;

    IDLookupTable()
        : m_nextInnerID(1)
        , m_objects()
        , m_freeList(UINT_MAX)
    {}

    bool Has(ID theID) const
    {
        return m_objects[theID & INDEX_MASK].theID == theID;
    }

    T& Lookup(ID theID)
    {
        return m_objects[theID & INDEX_MASK].u.value;
    }

    const T& Lookup(ID theID) const
    {
        return m_objects[theID & INDEX_MASK].u.value;
    }

    T& LookupRaw(ID rawIndex)
    {
        return m_objects[rawIndex].u.value;
    }

    const T& LookupRaw(ID rawIndex) const
    {
        return m_objects[rawIndex].u.value;
    }

    ID IDFromRawIndex(ID rawIndex) const
    {
        return m_objects[rawIndex].theID;
    }

    ID Add()
    {
        ASSERT(m_nextInnerID < INNER_ID_MASK);
        ID theID = 0;
        theID |= (m_nextInnerID++) << NumIndexBits;
        if (m_freeList == UINT_MAX) {
            ASSERT((ID)m_objects.size() < INDEX_MASK);
            theID |= (ID)m_objects.size();
            Object o;
            o.theID = theID;
            m_objects.push_back(o);
        } else {
            theID |= (ID)m_freeList;
            m_objects[m_freeList].theID = theID;
            m_freeList = m_objects[m_freeList].u.next;
        }
        return theID;
    }

    void Remove(ID theID)
    {
        Object& o = m_objects[theID & INDEX_MASK];
        // Invalidate the inner ID - keep the same index but fill the inner ID
        // bits with 1s.
        o.theID = (o.theID & INDEX_MASK) | (INNER_ID_MASK << NumIndexBits);

        o.u.next = m_freeList;
        m_freeList = (unsigned)(theID & INDEX_MASK);
    }

private:
    IDLookupTable(const IDLookupTable&);
    IDLookupTable& operator=(const IDLookupTable&);

    struct Object {
        ID theID;
        union {
            T value;
            unsigned next;
        } u;
    };

    unsigned m_nextInnerID;
    std::vector<Object> m_objects;
    unsigned m_freeList;
};

#endif // CORE_IDLOOKUPTABLE_H
