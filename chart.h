#ifndef CHART_H
#define CHART_H
#include <IChart.h>

#include <QString>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>

#include <memory>
#include <string>
#include <vector>

class Callout;
class Chart;

namespace QtCharts {
class QCategoryAxis;
class QValueAxis;
}

struct SeriesData : public QObject
{
    Q_OBJECT

public slots:
    void AddCallout(QPointF point, bool state);

public:
    SeriesData(Chart &chart, QtCharts::QLineSeries *series, std::string name, Fixpoint unit, Fixpoint centre, double min, double max);
    void SetRange();

    std::string GetText(Time time, double value) const;

    Chart  *mChart;
    QtCharts::QLineSeries *mSeries;
    QtCharts::QValueAxis  *mAxis;
    std::string mName;
    Fixpoint mUnit;
    double  mCentre;
    double  mMin;
    double  mMax;
};

class Chart : public QtCharts::QChartView, public IChart
{
    Q_OBJECT

public:
    Chart();
    virtual ~Chart();

    virtual bool Show() override final;

    virtual bool AddSegments(std::function<Time (double)> indexToTime, int64_t const *begin, int64_t const *end) override final;
    virtual void SetTimeTick(Interval timeTick) override final { mTimeTick = timeTick; }
    virtual void SetHorizontalRange(int64_t length) override final;

    virtual std::unique_ptr<ISeries> CreateSeries(Fixpoint axisCentre, char const *name) override final;

    void AddSeries(QtCharts::QLineSeries *series, std::string name, Fixpoint unit, Fixpoint centre, double min, double max);
    void AddCallout(SeriesData &seriesData, QPointF point, bool state);
    void RemoveCallout(Callout &callout);

    Interval GetTimeTick() const { return mTimeTick; }

private:
    struct Action
    {
        bool mComplete;
        Qt::MouseButton mButton;
        QPoint mBegin;
        QPoint mEnd;
    };

    virtual void resizeEvent(QResizeEvent *event) override;

    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;

    virtual void keyPressEvent(QKeyEvent *event) override;

    void AddCentreUnfixedData(QtCharts::QCategoryAxis *horizontalAxis, std::vector<std::unique_ptr<SeriesData>> const &data);
    void AddCentreFixedData(QtCharts::QCategoryAxis *horizontalAxis, std::vector<std::unique_ptr<SeriesData>> const &data);

    void KeepCallout();
    void UpdateCalloutsGeometry();

    QtCharts::QCategoryAxis * GetHorizontalAxis();
    void OnHorizontalAxisScroll(int direction);
    void OnHorizontalAxisZoom(int direction);
    void ResetActions();

    void OnKeyEscape();
    void OnToggleVerticalLine();

    Interval mTimeTick;

    std::function<Time (double)> mIndexToTime;
    std::vector<int64_t> mIndicesSegments;

    int64_t mHorizontalRangeLength;
    int32_t mHorizontalRangeScroll;
    int16_t mHorizontalRangeScaler;
    bool mVisible;

    QGraphicsLineItem *mVLine;

    std::vector<Action> mActions;

    std::vector<std::unique_ptr<SeriesData>> mSeries[2];

    std::vector<Callout *> mCallouts;
    Callout *mCallout;
};

#endif // CHART_H

