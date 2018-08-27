/****************************************************************************
** Meta object code from reading C++ file 'threaddialing.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../me909s-821_qt/threaddialing.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'threaddialing.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.8.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_threadDialing_t {
    QByteArrayData data[9];
    char stringdata0[108];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_threadDialing_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_threadDialing_t qt_meta_stringdata_threadDialing = {
    {
QT_MOC_LITERAL(0, 0, 13), // "threadDialing"
QT_MOC_LITERAL(1, 14, 13), // "signalDisplay"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 5), // "stage"
QT_MOC_LITERAL(4, 35, 6), // "result"
QT_MOC_LITERAL(5, 42, 23), // "slotMonitorTimerHandler"
QT_MOC_LITERAL(6, 66, 14), // "slotRunDialing"
QT_MOC_LITERAL(7, 81, 13), // "checkStageLTE"
QT_MOC_LITERAL(8, 95, 12) // "currentStage"

    },
    "threadDialing\0signalDisplay\0\0stage\0"
    "result\0slotMonitorTimerHandler\0"
    "slotRunDialing\0checkStageLTE\0currentStage"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_threadDialing[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   34,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       5,    0,   39,    2, 0x0a /* Public */,
       6,    1,   40,    2, 0x0a /* Public */,
       6,    0,   43,    2, 0x2a /* Public | MethodCloned */,

 // signals: parameters
    QMetaType::Void, QMetaType::Char, QMetaType::QString,    3,    4,

 // slots: parameters
    QMetaType::Int,
    QMetaType::Int, 0x80000000 | 7,    8,
    QMetaType::Int,

       0        // eod
};

void threadDialing::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        threadDialing *_t = static_cast<threadDialing *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->signalDisplay((*reinterpret_cast< char(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 1: { int _r = _t->slotMonitorTimerHandler();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = _r; }  break;
        case 2: { int _r = _t->slotRunDialing((*reinterpret_cast< checkStageLTE(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = _r; }  break;
        case 3: { int _r = _t->slotRunDialing();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = _r; }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (threadDialing::*_t)(char , QString );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&threadDialing::signalDisplay)) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject threadDialing::staticMetaObject = {
    { &QThread::staticMetaObject, qt_meta_stringdata_threadDialing.data,
      qt_meta_data_threadDialing,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *threadDialing::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *threadDialing::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_threadDialing.stringdata0))
        return static_cast<void*>(const_cast< threadDialing*>(this));
    return QThread::qt_metacast(_clname);
}

int threadDialing::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void threadDialing::signalDisplay(char _t1, QString _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
