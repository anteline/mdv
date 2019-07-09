#include <dataLoader.h>

#include <algorithm>
#include <iostream>


struct DataHeader
{
    uint32_t mEndianChecker;
    uint8_t mVersionNumber[4];
    uint32_t mNumSegments;
    uint32_t mNumSeries;
    Interval mTimeTick;
    Interval mDisplayRange;
};

template <typename T>
static T const * AlignTo(void const *ptr)
{
    static_assert(sizeof(void*) == sizeof(size_t), "Unexpected pointer size.");

    union
    {
        void const *mPtr;
        size_t mValue;
    } pointer = {ptr};

    size_t mask = alignof(T) - 1;
    if (pointer.mValue & mask)
        pointer.mValue = (pointer.mValue & ~mask) + alignof(T);

    return static_cast<T const *>(pointer.mPtr);
}

DataLoader::DataLoader(void const *data, size_t length)
:   mSegments(nullptr),
    mDisplayRange(0)
{
    if (data == nullptr or length <= sizeof(DataHeader))
    {
        std::cout << "Market data viewer requires more than " << sizeof(DataHeader) << " bytes of input data." << std::endl;
        return;
    }

    DataHeader const *header = static_cast<DataHeader const *>(data);
    if (header->mEndianChecker != 0xdeadbeef)
    {
        std::cout << "Market data viewer accepts only little-endian data." << std::endl;
        return;
    }

    if (header->mNumSegments == 0u)
    {
        std::cout << "At least one time segment is expected." << std::endl;
        return;
    }

    if (header->mNumSeries == 0u)
    {
        std::cout << "At least one timed series is expected." << std::endl;
        return;
    }

    if (header->mTimeTick < Nanoseconds(1))
    {
        std::cout << "Time tick is supposed to be positive." << std::endl;
        return;
    }

    if (header->mDisplayRange < Nanoseconds(1) or header->mDisplayRange % header->mTimeTick != Interval())
    {
        std::cout << "Display range is supposed to be positive multiple of time tick." << std::endl;
        return;
    }

    if (length < header->mNumSegments * sizeof(Segment) + sizeof(DataHeader))
    {
        std::cout << "Input data is less than expected, there is supposed to have " << header->mNumSegments << " segments." << std::endl;
        return;
    }

    Segment const *segments = reinterpret_cast<Segment const *>(header + 1);
    for (uint32_t i = 0; i < header->mNumSegments; ++i)
    {
        if (segments[i].second <= segments[i].first)
        {
            std::cout << "Invalid input data, " << i << "th segment's begin time is later than its end time." << std::endl;
            return;
        }
        if (i != 0 and segments[i].first < segments[i - 1].second)
        {
            std::cout << "Invalid input data, " << i << "th segment's begin time is earlier than previous one's end time." << std::endl;
            return;
        }
    }
    length -= (sizeof(DataHeader) + header->mNumSegments * sizeof(Segment));

    std::vector<int64_t> indicesRanges = CalcSegments(header->mTimeTick, segments, segments + header->mNumSegments);
    if (not indicesRanges.empty())
    {
        mIndicesRanges = std::move(indicesRanges);
        mSegments = segments;
        mTimeTick = header->mTimeTick;
    }

    void const *addr = segments + header->mNumSegments;
    std::vector<Series> series;
    series.reserve(header->mNumSeries);
    for (uint32_t i = 0; i < header->mNumSeries; ++i)
    {
        struct SeriesHeader
        {
            uint32_t mNumPoints;
            uint16_t mRightAxis;
            uint16_t mNameLength;
        };

        if (length <= sizeof(SeriesHeader))
        {
            std::cout << "Input data is less than expected, " << i << "th series header is incomplete." << std::endl;
            return;
        }
        SeriesHeader const *seriesHeader = static_cast<SeriesHeader const *>(addr);
        char const *name = reinterpret_cast<char const *>(seriesHeader + 1);
        std::pair<Time, Fixpoint> const *data = AlignTo<std::pair<Time, Fixpoint>>(name + seriesHeader->mNameLength + 1);

        size_t seriesLength = size_t(reinterpret_cast<char const *>(data + seriesHeader->mNumPoints) - static_cast<char const *>(addr));
        if (length < seriesLength)
        {
            std::cout << "Input is less than expected, " << i << "th series has less number of points than expected." << std::endl;
            return;
        }
        addr = data + seriesHeader->mNumPoints;
        length -= seriesLength;

        std::vector<std::pair<int64_t, double>> points = LoadPoints(data, data + seriesHeader->mNumPoints);
        if (not points.empty())
            series.push_back(Series{name, seriesHeader->mRightAxis == 0u, std::move(points)});
    }

    if (0 < length)
    {
        std::cout << "Input is more than expected, " << length << " bytes of unknown data are detected at the end." << std::endl;
        return;
    }

    mDisplayRange = header->mDisplayRange / header->mTimeTick;
    mSeries = std::move(series);
}

Time DataLoader::IndexToTime(double index)
{
    if (not(*this))
        return DistantPast();

    auto it = std::find_if(mIndicesRanges.begin(), mIndicesRanges.end(), [index](int64_t v) { return index < v; });
    int64_t idx = std::distance(mIndicesRanges.begin(), it) - 1;
    idx = std::min(int64_t(mIndicesRanges.size() - 2u), std::max(0l, idx));
    return mSegments[idx].first + Nanoseconds(int64_t((index - mIndicesRanges[idx]) * mTimeTick.TotalNanoseconds()));
}

std::vector<int64_t> DataLoader::CalcSegments(Interval timeTick, Segment const *begin, Segment const *end)
{
    std::vector<int64_t> segments;
    if (Nanoseconds(0) < timeTick and begin != nullptr and end != nullptr and begin < end)
    {
        segments.reserve(size_t(end - begin) + 1u);
        segments.push_back(0);
        for (Segment const *it = begin; it < end; ++it)
        {
            Interval interval = it->second - it->first;
            if (interval % timeTick != Interval())
            {
                std::cout << "Invalid input data, time range is not multiples of time tick. TimeTick=" << timeTick
                        << " Begin=" << it->first.ToString().data() << " End=" << it->second.ToString().data() << std::endl;
                return std::vector<int64_t>();
            }
            segments.push_back(segments.back() + interval / timeTick);
        }
    }
    return std::move(segments);
}

std::vector<std::pair<int64_t, double>> DataLoader::LoadPoints(Data const *begin, Data const *end) const
{
    std::vector<std::pair<int64_t, double>> rtn;
    rtn.reserve(size_t(end - begin));

    size_t segIdx = 0;
    for (Data const *it = begin; it < end; ++it)
    {
        if (it != begin and it->first <= (it - 1)->first)
            return std::vector<std::pair<int64_t, double>>();

        while (mSegments[segIdx].second < it->first)
        {
            if (mIndicesRanges.size() - 2 < ++segIdx)
                return std::vector<std::pair<int64_t, double>>();
        }

        if (it->first < mSegments[segIdx].first)
            return std::vector<std::pair<int64_t, double>>();

        Interval interval = it->first - mSegments[segIdx].first;
        if (interval % mTimeTick != Interval())
            return std::vector<std::pair<int64_t, double>>();
        rtn.push_back(std::make_pair(mIndicesRanges[segIdx] + interval / mTimeTick, double(it->second)));
    }
    return std::move(rtn);
}

