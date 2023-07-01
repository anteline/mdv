#ifndef DATA_LOADER_BASE_H
#define DATA_LOADER_BASE_H
#include <IDataLoader.h>
#include <vector>

struct PointData
{
    Time mTime;
    Fixpoint mValue;
};

class DataLoaderBase : public IDataLoader
{
public:
    virtual bool IsValid() const override final { return 0 < mDisplayRange; }

    virtual int64_t GetDisplayRange() const override final { return mDisplayRange; }

    virtual Interval GetTimeTick() const override final { return mTimeTick; }

    virtual int64_t const * GetIndicesRangesBegin() const override final { return mIndicesRanges.data(); }
    virtual int64_t const * GetIndicesRangesEnd() const override final { return mIndicesRanges.data() + mIndicesRanges.size(); }

    virtual Segment const * GetSegments() const override final { return mSegments.data(); }

    virtual std::function<Time (double)> GetTimeCalculator() const override final;

protected:
    enum Shape
    {
        Curve = 1,
        Spike = 2,
        Step = 3
    };

    DataLoaderBase();
    virtual ~DataLoaderBase();

    bool InsertSegments(Interval timeTick, Segment const *begin, Segment const *end);
    bool InsertSegments(Interval timeTick, std::vector<Segment> segments);

    std::vector<Point> LoadPoints(char const *series, int32_t shape, Fixpoint centre, PointData const *begin, PointData const *end) const;
    std::vector<Point> LoadPoints(char const *series, int32_t shape, Fixpoint centre, std::vector<PointData> const &data) const;

    template <typename Actor>
    bool LoadPoints(char const *series, Actor const &actor, PointData const *begin, PointData const *end) const;

    int64_t mDisplayRange;

    Interval mTimeTick;

    std::vector<int64_t> mIndicesRanges;
    std::vector<Segment> mSegments;
};

#endif // DATA_LOADER_BASE_H

