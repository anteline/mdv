#include <QtWidgets/QApplication>
#include <chart.h>
#include <dataLoader.h>
#include <file.hpp>
#include <time.hpp>

#include <iostream>


int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage: mdv <input-file>" << std::endl;
        return -1;
    }

    File file(argv[1]);
    if (not(file))
    {
        std::cout << "Failed to open input file " << argv[1] << std::endl;
        return -1;
    }

    DataLoader data(file.GetContent(), file.GetLength());
    std::pair<int64_t const *, int64_t const *> indicesRanges = data.GetIndicesRanges();

    QApplication a(argc, argv);

    std::unique_ptr<Chart> chart(new Chart);
    chart->AddSegments([&data](double idx) { return data.IndexToTime(idx); }, indicesRanges.first, indicesRanges.second);
    chart->SetHorizontalRange(data.GetDisplayRange());

    typedef std::pair<int64_t, double> Point;
    data.ForeachSeries([&chart, &data](char const *name, Fixpoint axisCentre, Point const *begin, Point const *end)
    {
        std::unique_ptr<IChart::ISeries> series = chart->CreateSeries(axisCentre, name);
        for (Point const *point = begin; point < end; ++point)
            series->Append(point->first, point->second);
        series->Commit();
    });

    chart->Show();

    return a.exec();
}

