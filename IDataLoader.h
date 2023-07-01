#ifndef IDATA_LOADER_H
#define IDATA_LOADER_H
#include <fixpoint.hpp>
#include <time.hpp>
#include <functional>
#include <stdint.h>

struct Point
{
    int64_t x;
    Fixpoint y;
};

struct Segment
{
    Time mOpen;
    Time mClose;
};

class IDataLoader
{
public:
    class ISeriesRetriever
    {
    public:
        virtual ~ISeriesRetriever() { }
        virtual void OnSeries(char const *name, char const *group, Fixpoint axisCentre, Point const *begin, Point const *end) = 0;
    };

    virtual ~IDataLoader() { }

    virtual bool IsValid() const = 0;

    virtual Interval GetTimeTick() const = 0;
    virtual int64_t GetDisplayRange() const  = 0;

    virtual int64_t const * GetIndicesRangesBegin() const = 0;
    virtual int64_t const * GetIndicesRangesEnd() const = 0;

    virtual Segment const * GetSegments() const = 0;

    virtual std::function<Time (double)> GetTimeCalculator() const = 0;

    virtual void RetrieveSeries(ISeriesRetriever &retriever) const = 0;
};

#endif // IDATA_LOADER_H

