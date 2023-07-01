QT += charts
CONFIG += c++11

LIBS += -Wl,-rpath,/usr/local/lib64 -L/usr/local/lib64 -lhdf5

HEADERS +=              \
    IChart.h            \
    IDataLoader.h       \
    callout.h           \
    chart.h             \
    dataLoader.h        \
    dataLoaderBase.h    \
    file.hpp            \
    h5DataLoader.h      \
    time.hpp            \
    fixpoint.hpp

SOURCES +=              \
    main.cpp            \
    callout.cpp         \
    chart.cpp           \
    dataLoader.cpp      \
    dataLoaderBase.cpp  \
    file.cpp            \
    h5DataLoader.cpp    \
    time.cpp            \
    fixpoint.cpp

