#ifndef ICHART_H
#define ICHART_H
#include <fixpoint.hpp>
#include <time.hpp>
#include <functional>
#include <memory>
#include <stdint.h>

class IChart
{
public:
    enum Axis
    {
        Horizontal,
        Left,
        Right
    };

    class ISeries
    {
    public:
        virtual ~ISeries() { }

        virtual void Commit() = 0;
        virtual void Append(int64_t x, Fixpoint y) = 0;
    };

    virtual ~IChart() { }

    virtual bool Show() = 0;

    virtual bool AddSegments(std::function<Time (double)> indexToTime, int64_t const *begin, int64_t const *end) = 0;
    virtual void SetTimeTick(Interval timeTick) = 0;
    virtual void SetHorizontalRange(int64_t length) = 0;

    virtual std::unique_ptr<ISeries> CreateSeries(Fixpoint axisCentre, char const *name, char const *group) = 0;
};

#endif // ICHART_H

