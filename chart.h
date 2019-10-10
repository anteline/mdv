#ifndef CHART_H
#define CHART_H
#include <IChart.h>

#include <QString>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>

#include <memory>
#include <vector>


namespace QtCharts {
class QCategoryAxis;
}

class Chart : private QtCharts::QChartView, public IChart
{
    Q_OBJECT

public:
    Chart();
    virtual ~Chart();

    virtual bool Show() override final;

    virtual bool AddSegments(std::function<Time (double)> indexToTime, int64_t const *begin, int64_t const *end) override final;
    virtual void SetHorizontalRange(int64_t length) override final;

    virtual std::unique_ptr<ISeries> CreateSeries(Axis axis, char const *name) override final;

private:
    class Series;

    struct Action
    {
        bool mComplete;
        Qt::MouseButton mButton;
        QPoint mBegin;
        QPoint mEnd;
    };

    struct SeriesData
    {
        QtCharts::QLineSeries *mSeries;
        double mMin;
        double mMax;
    };

    virtual void resizeEvent(QResizeEvent *event) override;

    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;

    virtual void keyPressEvent(QKeyEvent *event) override;

    void AddSeries(Axis axis, SeriesData series);
    void CreateCoordinators();
    void SetCoordinators();

    QtCharts::QCategoryAxis * GetHorizontalAxis();
    void OnHorizontalAxisScroll(int direction);
    void OnHorizontalAxisZoom(int direction);
    void ResetActions();

    void OnKeyEscape();
    void OnKeyF(int idx);

    std::function<Time (double)> mIndexToTime;
    std::vector<int64_t> mIndicesSegments;

    int64_t mHorizontalRangeLength;
    int32_t mHorizontalRangeScroll;
    int16_t mHorizontalRangeScaler;
    bool mVisible;
    bool mAxisLocked[2];

    std::unique_ptr<QGraphicsSimpleTextItem> mValues[3];

    std::vector<Action> mActions;

    std::vector<SeriesData> mSeries[2];
};

#endif // CHART_H

