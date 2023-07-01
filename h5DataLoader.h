#ifndef H5_DATA_LOADER_H
#define H5_DATA_LOADER_H
#include <dataLoaderBase.h>
#include <memory>
#include <vector>

class H5DataLoader final : public DataLoaderBase
{
public:
    explicit H5DataLoader(char const *fname);
    virtual ~H5DataLoader();

    virtual void RetrieveSeries(ISeriesRetriever &retriever) const override final;

private:
    struct Series
    {
        std::unique_ptr<char []> mName;
        char const *mGroup;
        Fixpoint    mAxisCentre;
        std::vector<Point> mData;
    };

    std::vector<Series> mSeries;
};

#endif // H5_DATA_LOADER_H

