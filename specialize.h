#pragma once

// This header provides a cleaner mechanism for declaring template
// specializations via typedefs without lots of code clutter.
//
// Note:  If std::tuple is not used (because Qt does not provide an
// equivalent), then these macros are interchangeable.

//#ifdef Q_OBJECT
//#define USE_QT_CONTAINERS
////#include <QtGlobal>
////#if QT_VERSION >= 0x050000
////// Qt containers are deprecated as of 5.0
////// https://stackoverflow.com/questions/35525588/is-there-a-qpair-class-but-for-three-items-instead-of-two
////#undef USE_QT_CONTAINERS
////#endif
//#endif

#ifdef __cplusplus
#define SPECIALIZE_ITER(name) \
    typedef name::iterator          name##Iter; \
    typedef name::const_iterator    name##ConstIter;

#ifdef QT_VERSION
// We're building with Qt -- Use Qt-specific container types
#include <QVector>
#define SPECIALIZE_VECTOR(type, name) \
    typedef QVector<type>   name##Vector; \
    SPECIALIZE_ITER(name##Vector)

#include <QMap>
#define SPECIALIZE_MAP(key, value, name) \
    typedef QMap<key, value>   name##Map; \
    SPECIALIZE_ITER(name##Map)

#define map_key(iter)    iter.key()
#define map_value(iter)    iter.value()

#include <QList>
#define SPECIALIZE_LIST(type, name) \
    typedef QList<type>   name##List; \
    SPECIALIZE_ITER(name##List)

#include <QSharedPointer>
#define SPECIALIZE_SHAREDPTR(type, name) \
    typedef QSharedPointer<type>   name##Pointer;

#include <QWeakPointer>
#define SPECIALIZE_WEAKPTR(type, name) \
    typedef QWeakPointer<type>   name##WeakPointer;

#include <QQueue>
#define SPECIALIZE_QUEUE(type, name) \
    typedef QQueue<type>   name##Queue;

#include <QPair>
#define SPECIALIZE_PAIR(type1, type2, name) \
    typedef QPair<type1, type2>   name##Pair;

#include <QSet>
#define SPECIALIZE_SET(type, name) \
    typedef QSet<type>   name##Set;

#else   // Using STL

#include <vector>
#define SPECIALIZE_VECTOR(type, name) \
    typedef std::vector<type>   name##Vector; \
    SPECIALIZE_ITER(name##Vector)

#include <map>
#define SPECIALIZE_MAP(key, value, name) \
    typedef std::map<key, value>   name##Map; \
    SPECIALIZE_ITER(name##Map)

#define map_key(iter)    iter->first
#define map_value(iter)    iter->second

#include <list>
#define SPECIALIZE_LIST(type, name) \
    typedef std::list<type>   name##List; \
    SPECIALIZE_ITER(name##List)

#include <memory>
#define SPECIALIZE_SHAREDPTR(type, name) \
    typedef std::shared_ptr<type>   name##Pointer;

#define SPECIALIZE_WEAKPTR(type, name) \
    typedef std::weak_ptr<type>   name##WeakPointer;

#include <queue>
#define SPECIALIZE_QUEUE(type, name) \
    typedef std::queue<type>   name##Queue;

#include <set>
#define SPECIALIZE_SET(type, name) \
    typedef std::set<type>   name##Set;

#include <utility>
#define SPECIALIZE_PAIR(type1, type2, name) \
    typedef std::pair<type1, type2>   name##Pair;

#include <tuple>
// for a 2-tuple, use Pair
#define SPECIALIZE_TUPLE3(type1, type2, type3, name) \
    typedef std::tuple<type1, type2, type3>   name##Tuple3;
#define SPECIALIZE_TUPLE4(type1, type2, type3, type4, name) \
    typedef std::tuple<type1, type2, type3, type4>   name##Tuple4;
#define SPECIALIZE_TUPLE5(type1, type2, type3, type4, type5, name) \
    typedef std::tuple<type1, type2, type3, type4, type5>   name##Tuple5;

#endif  // QT_VERSION

#endif  // __cplusplus
