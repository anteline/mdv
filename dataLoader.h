#ifndef DATA_LOADER_H
#define DATA_LOADER_H
#include <dataLoaderBase.h>
#include <file.hpp>
#include <vector>


// Data format is supposed to be:
// 0xdeadbeef:int32_t,versionNo:uint8_t[4]
// numSegments:uint32_t,numSeries:uint32_t
// TimeTick:Interval
// DisplayRange:Interval
// [segmentBegin:Time,segmentEnd:Time] * numSegments
// [centre:Fixpoint,numPoints:uint32_t,nameLength:uint32_t,name:char[nameLength],[x:Time,y:Fixpoint] * numPoints] * numSeries
class DataLoader final : public DataLoaderBase
{
public:
    explicit DataLoader(char const *fname);
    virtual ~DataLoader();

    virtual void RetrieveSeries(ISeriesRetriever &retriever) const override final;

protected:
    File mFile;

    struct Series
    {
        char const *mName;
        char const *mGroup;
        Fixpoint    mAxisCentre;
        std::vector<Point> mData;
    };

    std::vector<Series> mSeries;
};

#endif // DATA_LOADER_H

