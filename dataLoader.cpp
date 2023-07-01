#include <dataLoader.h>

#include <algorithm>
#include <iostream>


struct DataHeader
{
    uint32_t mEndianChecker;
    uint8_t  mVersionNumber[4];
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

DataLoader::DataLoader(char const *fname)
:   mFile(fname)
{
    if (not(mFile))
    {
        std::cout << "Failed to open input data file " << fname << std::endl;
        return;
    }

    void const *data = mFile.GetContent();
    size_t length = mFile.GetLength();
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
        if (segments[i].mClose <= segments[i].mOpen)
        {
            std::cout << "Invalid input data, " << i << "th segment's begin time is later than its end time." << std::endl;
            return;
        }
        if (i != 0 and segments[i].mOpen < segments[i - 1].mClose)
        {
            std::cout << "Invalid input data, " << i << "th segment's begin time is earlier than previous one's end time." << std::endl;
            return;
        }
    }
    length -= (sizeof(DataHeader) + header->mNumSegments * sizeof(Segment));

    if (not DataLoaderBase::InsertSegments(header->mTimeTick, segments, segments + header->mNumSegments))
        return;

    void const *addr = segments + header->mNumSegments;
    std::vector<Series> series;
    series.reserve(header->mNumSeries);
    for (uint32_t i = 0; i < header->mNumSeries; ++i)
    {
        struct SeriesHeader
        {
            Fixpoint mAxisCentre;
            uint32_t mNumPoints;
            uint16_t mNameLength;
            uint16_t mGroupLength;
        };

        if (length <= sizeof(SeriesHeader))
        {
            std::cout << "Input data is less than expected, " << i << "th series header is incomplete." << std::endl;
            return;
        }
        SeriesHeader const *seriesHeader = static_cast<SeriesHeader const *>(addr);
        char const *name = reinterpret_cast<char const *>(seriesHeader + 1);
        char const *group = name + seriesHeader->mNameLength + int(seriesHeader->mGroupLength != 0u);
        PointData const *data = AlignTo<PointData>(group + seriesHeader->mGroupLength + 1);

        size_t seriesLength = size_t(reinterpret_cast<char const *>(data + seriesHeader->mNumPoints) - static_cast<char const *>(addr));
        if (length < seriesLength)
        {
            std::cout << "Input is less than expected, " << i << "th series has less number of points than expected." << std::endl;
            return;
        }
        addr = data + seriesHeader->mNumPoints;
        length -= seriesLength;

        std::vector<Point> points = LoadPoints(name, 1, seriesHeader->mAxisCentre, data, data + seriesHeader->mNumPoints);
        if (not points.empty())
            series.push_back(Series{name, group, seriesHeader->mAxisCentre, std::move(points)});
    }

    if (0 < length)
    {
        std::cout << "Input is more than expected, " << length << " bytes of unknown data are detected at the end." << std::endl;
        return;
    }

    mDisplayRange = header->mDisplayRange / header->mTimeTick;
    mSeries = std::move(series);
}

DataLoader::~DataLoader()
{
}

void DataLoader::RetrieveSeries(ISeriesRetriever &retriever) const
{
    for (Series const &series : mSeries)
        retriever.OnSeries(series.mName, series.mGroup, series.mAxisCentre, &series.mData[0], &series.mData[series.mData.size()]);
}

