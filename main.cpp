#include <QtWidgets/QApplication>
#include <chart.h>
#include <dataLoader.h>
#include <h5DataLoader.h>
#include <time.hpp>

#include <iostream>

class DataChart : IDataLoader::ISeriesRetriever
{
public:
    static std::unique_ptr<IDataLoader> CreateDataLoader(char const *fname)
    {
        std::unique_ptr<IDataLoader> data;
        if (fname != nullptr)
        {
            size_t len = strlen(fname);
            if (3 < len and memcmp(fname + len - 3, ".h5", 3) == 0)
            {
                data.reset(new H5DataLoader(fname));
            }
            else
            {
                data.reset(new DataLoader(fname));
            }

            if (not data->IsValid())
                data.reset();
        }
        return std::move(data);
    }

    explicit DataChart(char const *fname)
    {
        mData = CreateDataLoader(fname);
    }

    bool IsValid() const { return mData != nullptr; }

    virtual void OnSeries(char const *name, char const *group, Fixpoint axisCentre, Point const *begin, Point const *end) override final
    {
        std::unique_ptr<IChart::ISeries> series = mChart->CreateSeries(axisCentre, name, group);
        for (Point const *point = begin; point < end; ++point)
            series->Append(point->x, point->y);
        series->Commit();
    }

    void Show()
    {
        mChart.reset(new Chart);
        mChart->AddSegments(mData->GetTimeCalculator(), mData->GetIndicesRangesBegin(), mData->GetIndicesRangesEnd());
        mChart->SetTimeTick(mData->GetTimeTick());
        mChart->SetHorizontalRange(mData->GetDisplayRange());

        mData->RetrieveSeries(*this);
        mChart->Show();
    }

private:
    std::unique_ptr<IDataLoader> mData;
    std::unique_ptr<Chart> mChart;
};

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage: mdv <input-file>" << std::endl;
        return -1;
    }

    DataChart chart(argv[1]);
    if (not chart.IsValid())
        return -1;

    QApplication a(argc, argv);
    chart.Show();
    return a.exec();
}

