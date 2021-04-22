#pragma once

// This header provides a cleaner mechanism for declaring template
// specializations via typedefs without lots of code clutter.

#ifdef __cplusplus
#define SPECIALIZE_ITER(name) \
    typedef name::iterator          name##Iter; \
    typedef name::const_iterator    name##ConstIter;

#ifdef QT_VERSION
// We're building with Qt -- Use Qt-specific container types
#include <QVector>
#define SPECIALIZE_VECTOR(type, name) \
    using name##Vector = QVector<type>; \
    SPECIALIZE_ITER(name##Vector)

#include <QMap>
#define SPECIALIZE_MAP(key, value, name) \
    using name##Map = QMap<key, value>; \
    SPECIALIZE_ITER(name##Map)

#define map_key(iter)    iter.key()
#define map_value(iter)    iter.value()

#include <QList>
#define SPECIALIZE_LIST(type, name) \
    using name##List = QList<type>; \
    SPECIALIZE_ITER(name##List)

#include <QSharedPointer>
#define SPECIALIZE_SHAREDPTR(type, name) \
    using name##Pointer = QSharedPointer<type>;

#include <QWeakPointer>
#define SPECIALIZE_WEAKPTR(type, name) \
    using name##WeakPointer = QWeakPointer<type>;

#include <QQueue>
#define SPECIALIZE_QUEUE(type, name) \
    using name##Queue = QQueue<type>;

#include <QPair>
#define SPECIALIZE_PAIR(type1, type2, name) \
    using name##Pair = QPair<type1, type2>;

#include <QSet>
#define SPECIALIZE_SET(type, name) \
    using name##Set = QSet<type>;

#else   // Using STL

#include <vector>
#define SPECIALIZE_VECTOR(type, name) \
    using name##Vector = std::vector<type>; \
    SPECIALIZE_ITER(name##Vector)

#include <map>
#define SPECIALIZE_MAP(key, value, name) \
    using name##Map = std::map<key, value>; \
    SPECIALIZE_ITER(name##Map)

#define map_key(iter)    iter->first
#define map_value(iter)    iter->second

#include <list>
#define SPECIALIZE_LIST(type, name) \
    using name##List = std::list<type>; \
    SPECIALIZE_ITER(name##List)

#include <memory>
#define SPECIALIZE_SHAREDPTR(type, name) \
    using name##Pointer = std::shared_ptr<type>;

#define SPECIALIZE_WEAKPTR(type, name) \
    using name##WeakPointer = std::weak_ptr<type>;

#include <queue>
#define SPECIALIZE_QUEUE(type, name) \
    using name##Queue = std::queue<type>;

#include <set>
#define SPECIALIZE_SET(type, name) \
    using name##Set = std::set<type>;

#include <utility>
#define SPECIALIZE_PAIR(type1, type2, name) \
    using name##Pair = std::pair<type1, type2>;

#endif  // QT_VERSION

#endif  // __cplusplus
