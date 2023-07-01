#include <dataLoaderBase.h>
#include <algorithm>
#include <iostream>

DataLoaderBase::DataLoaderBase()
:   mDisplayRange(0)
{
}

DataLoaderBase::~DataLoaderBase()
{
}

bool DataLoaderBase::InsertSegments(Interval timeTick, Segment const *begin, Segment const *end)
{
    return begin != nullptr and begin < end and InsertSegments(timeTick, std::vector<Segment>(begin, end));
}

bool DataLoaderBase::InsertSegments(Interval timeTick, std::vector<Segment> segments)
{
    if (timeTick <= Nanoseconds(0) or segments.empty())
        return false;

    std::vector<int64_t> ranges;
    ranges.reserve(segments.size() + 1u);
    ranges.push_back(0);
    for (Segment const *it = segments.data(); it < segments.data() + segments.size(); ++it)
    {
        if (it->mClose <= it->mOpen)
        {
            std::cout << "Invalid input data, segment open time is supposed to be earlier than close time. TimeTick=" << timeTick
                    << " Open=" << it->mOpen.ToString().data()
                    << " Close=" << it->mClose.ToString().data()
                    << std::endl;
            return false;
        }

        if (segments.data() < it and it->mOpen <= (it - 1)->mClose)
        {
            std::cout << "Invalid input data, segment open time is supposed to be later than previous close time. TimeTick=" << timeTick
                    << " Open=" << it->mOpen.ToString().data()
                    << " PrevClose=" << (it - 1)->mClose.ToString().data()
                    << std::endl;
            return false;
        }

        Interval interval = it->mClose - it->mOpen;
        if (interval % timeTick != Interval())
        {
            std::cout << "Invalid input data, segment time range is not multiples of time tick. TimeTick=" << timeTick
                    << " Open=" << it->mOpen.ToString().data()
                    << " Close=" << it->mClose.ToString().data()
                    << std::endl;
            return false;
        }
        ranges.push_back(ranges.back() + interval / timeTick);
    }

    mTimeTick = timeTick;
    mIndicesRanges = std::move(ranges);
    mSegments = std::move(segments);
    return true;
}

std::vector<Point> DataLoaderBase::LoadPoints(char const *series, int32_t shape, Fixpoint centre, std::vector<PointData> const &data) const
{
    return LoadPoints(series, shape, centre, data.data(), data.data() + data.size());
}

template <typename Actor>
bool DataLoaderBase::LoadPoints(char const *series, Actor const &actor, PointData const *begin, PointData const *end) const
{
    size_t segIdx = 0;
    for (PointData const *it = begin; it < end; ++it)
    {
        if (it != begin and it->mTime <= (it - 1)->mTime)
        {
            std::cout << "Invalid input data, data time is supposed to be later than previous data time. Series=" << series
                    << " Time=" << it->mTime.ToString().data() << " PrevTime=" << it->mTime.ToString().data() << "\n";
            return false;
        }

        while (mSegments[segIdx].mClose < it->mTime)
        {
            if (mSegments.size() <= ++segIdx)
            {
                std::cout << "Invalid input data, data time is later than last trading range. Series=" << series
                        << " Time=" << it->mTime.ToString().data() << "\n";
                return false;
            }
        }

        if (it->mTime < mSegments[segIdx].mOpen)
        {
            std::cout << "Invalid input data, data time is not in any trading range. Series=" << series
                    << " Time=" << it->mTime.ToString().data() << "\n";
            return false;
        }

        Interval interval = it->mTime - mSegments[segIdx].mOpen;
        if (interval % mTimeTick != Interval())
        {
            std::cout << "Invalid input data, data time is not at proper time tick. TimeTick=" << mTimeTick
                << " Series=" << series << " Time=" << it->mTime.ToString().data() << "\n";
            return false;
        }
        actor(segIdx, interval / mTimeTick, it->mValue);
    }
    return true;
}

std::vector<Point> DataLoaderBase::LoadPoints(char const *series, int32_t shape, Fixpoint centre, PointData const *begin, PointData const *end) const
{
    std::vector<Point> rtn;
    rtn.reserve(size_t(end - begin));

    if (shape == Shape::Curve)
    {
        auto actor = [this, &rtn](size_t idx, int64_t offset, Fixpoint value)
        {
            rtn.push_back(Point{ mIndicesRanges[idx] + offset, value });
        };
        if (LoadPoints(series, actor, begin, end))
            return std::move(rtn);
    }
    else if (shape == Shape::Spike)
    {
        auto actor = [this, centre, &rtn](size_t idx, int64_t offset, Fixpoint value)
        {
            if (value == centre or offset < 0 or mIndicesRanges.size() < idx + 2u or mIndicesRanges[idx + 1] < mIndicesRanges[idx] + offset)
                return;

            offset -= int(offset != 0) + int(mIndicesRanges[idx] + offset == mIndicesRanges[idx + 1]);
            rtn.push_back(Point{ mIndicesRanges[idx] + offset, centre });
            rtn.push_back(Point{ mIndicesRanges[idx] + offset + 1, value });
            rtn.push_back(Point{ mIndicesRanges[idx] + offset + 2, centre });
        };
        if (LoadPoints(series, actor, begin, end))
            return std::move(rtn);
    }
    else if (shape == Shape::Step)
    {
        Fixpoint prev = begin->mValue;
        auto actor = [this, &prev, &rtn](size_t idx, int64_t offset, Fixpoint value)
        {
            offset -= int(offset != 0);
            rtn.push_back(Point{ mIndicesRanges[idx] + offset, prev });
            rtn.push_back(Point{ mIndicesRanges[idx] + offset + 1, value });
            prev = value;
        };
        if (LoadPoints(series, actor, begin, end))
            return std::move(rtn);
    }
    else
    {
        std::cout << "Invalid input data, unexpected data shape. Series=" << series << " Shape=" << shape << "\n";
    }
    return std::vector<Point>();
}

std::function<Time (double)> DataLoaderBase::GetTimeCalculator() const
{
    return [this](double index) -> Time
    {
        auto it = std::find_if(mIndicesRanges.begin(), mIndicesRanges.end(), [index](int64_t v) { return index < v; });
        ssize_t idx = std::distance(mIndicesRanges.begin(), it) - 1;
        idx = std::min(ssize_t(mIndicesRanges.size() - 2u), std::max(0l, idx));
        return mSegments[idx].mOpen + Nanoseconds(int64_t((index - mIndicesRanges[idx]) * mTimeTick.TotalNanoseconds()));
    };
}

