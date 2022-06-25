#ifndef DATA_LOADER_H
#define DATA_LOADER_H
#include <fixpoint.hpp>
#include <time.hpp>

#include <vector>
#include <stdint.h>


// Data format is supposed to be:
// 0xdeadbeef:int32_t,versionNo:uint8_t[4]
// numSegments:uint32_t,numSeries:uint32_t
// TimeTick:Interval
// DisplayRange:Interval
// [segmentBegin:Time,segmentEnd:Time] * numSegments
// [centre:Fixpoint,numPoints:uint32_t,nameLength:uint32_t,name:char[nameLength],[x:Time,y:Fixpoint] * numPoints] * numSeries
class DataLoader final
{
public:
    DataLoader(void const *data, size_t length);

    operator bool() const noexcept { return 0 < mDisplayRange; }

    Time IndexToTime(double index);
    Interval GetTimeTick() const noexcept { return mTimeTick; }

    std::pair<int64_t const *, int64_t const *> GetIndicesRanges() const
    {
        return std::make_pair(&mIndicesRanges[0], &mIndicesRanges[mIndicesRanges.size()]);
    }
    int64_t GetDisplayRange() const { return mDisplayRange; }

    template <typename Actor>
    void ForeachSeries(Actor const &actor) const
    {
        for (Series const &series : mSeries)
            actor(series.mName, series.mGroup, series.mAxisCentre, &series.mData[0], &series.mData[series.mData.size()]);
    }

protected:
    typedef std::pair<Time, Time> Segment;
    typedef std::pair<Time, Fixpoint> Data;

    static std::vector<int64_t> CalcSegments(Interval timeTick, Segment const *begin, Segment const *end);
    std::vector<std::pair<int64_t, Fixpoint>> LoadPoints(Data const *begin, Data const *end) const;

    struct Series
    {
        char const *mName;
        char const *mGroup;
        Fixpoint    mAxisCentre;
        std::vector<std::pair<int64_t, Fixpoint>> mData;
    };

    std::vector<int64_t> mIndicesRanges;
    Segment const *mSegments;

    Interval mTimeTick;
    int64_t mDisplayRange;

    std::vector<Series> mSeries;
};

#endif // DATA_LOADER_H

