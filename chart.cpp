#include <chart.h>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QGraphicsScene>
#include <QtCharts/QCategoryAxis>
#include <QtCharts/QChart>
#include <QtWidgets/QGraphicsTextItem>
#include <QtGui/QMouseEvent>
#include <algorithm>
#include <limits>

#include <iostream>


static char const *sValuePrefix[] = {"Bottom: ", "Left: ", "Right: "};

static QString ToString(Time time, size_t length)
{
    std::array<char, 32> str = time.ToString();
    if (length < sizeof(str))
        str[length] = char(0);
    return QString(str.data());
}

Chart::Chart()
:   QtCharts::QChartView(new QtCharts::QChart, nullptr),
    mHorizontalRangeLength(std::numeric_limits<int64_t>::max()),
    mHorizontalRangeScroll(0),
    mHorizontalRangeScaler(1),
    mVisible(false)
{
    chart()->setMinimumSize(640, 480);
    chart()->legend()->setVisible(true);
    chart()->legend()->setAlignment(Qt::AlignBottom);

    setRenderHint(QPainter::Antialiasing);
}

Chart::~Chart()
{
}

bool Chart::Show()
{
    if (mVisible)
        return true;

    if (mIndexToTime and (not(mSeries[0].empty()) or not(mSeries[1].empty())))
    {
        SetCoordinators();

        QtCharts::QCategoryAxis *horizontalAxis = new QtCharts::QCategoryAxis;
        horizontalAxis->setLabelsPosition(QtCharts::QCategoryAxis::AxisLabelsPositionOnValue);

        for (size_t i = 0; i < mIndicesSegments.size(); ++i)
        {
            int64_t index = mIndicesSegments[i];
            QString label = ToString(mIndexToTime(index), 16u);
            horizontalAxis->append(label, index);
        }
        if (mHorizontalRangeLength < mIndicesSegments.back() - mIndicesSegments.front())
        {
            horizontalAxis->setMin(mIndicesSegments.back() - mHorizontalRangeLength);
            horizontalAxis->setMax(mIndicesSegments.back());
        }
        chart()->addAxis(horizontalAxis, Qt::AlignBottom);

        for (Axis axis : {Axis::Left, Axis::Right})
        {
            if (not mSeries[axis - 1].empty())
            {
                double min = std::numeric_limits<double>::max(), max = -std::numeric_limits<double>::max();

                QtCharts::QValueAxis *verticalAxis = new QtCharts::QValueAxis;
                chart()->addAxis(verticalAxis, axis == Axis::Left ? Qt::AlignLeft : Qt::AlignRight);
                for (SeriesData &seriesData : mSeries[axis - 1])
                {
                    chart()->addSeries(seriesData.mSeries);
                    seriesData.mSeries->attachAxis(horizontalAxis);
                    seriesData.mSeries->attachAxis(verticalAxis);

                    min = std::min(min, seriesData.mMin);
                    max = std::max(max, seriesData.mMax);
                }
                verticalAxis->setMin(min);
                verticalAxis->setMax(max);
            }
        }

        show();
        mVisible = true;
        return true;
    }
    return false;
}

bool Chart::AddSegments(std::function<Time (double)> indexToTime, int64_t const *begin, int64_t const *end)
{
    if (not(indexToTime) or mIndexToTime)
        return false;
    mIndexToTime = std::move(indexToTime);

    if (begin != nullptr and end != nullptr and begin < end)
    {
        mIndicesSegments = std::vector<int64_t>(begin, end);
        std::sort(mIndicesSegments.begin(), mIndicesSegments.end());
    }
    return true;
}

void Chart::SetHorizontalRange(int64_t length)
{
    if (0 < length)
        mHorizontalRangeLength = length;
}

class Chart::Series : public IChart::ISeries
{
public:
    explicit Series(Chart &chart, Axis axis, char const *name)
    :   mSeries(new QtCharts::QLineSeries),
        mChart(chart),
        mAxis(axis),
        mMin(std::numeric_limits<double>::max()),
        mMax(-std::numeric_limits<double>::max())
    {
        mSeries->setName(name);
    }

    virtual void Commit() override final
    {
        if (mSeries != nullptr)
            mChart.AddSeries(mAxis, SeriesData{mSeries.release(), mMin, mMax});
    }

    virtual void Append(int64_t x, double y) override final
    {
        if (mSeries != nullptr)
        {
            mSeries->append(x, y);
            mMin = std::min(mMin, y);
            mMax = std::max(mMax, y);
        }
    }

private:
    std::unique_ptr<QtCharts::QLineSeries> mSeries;
    Chart &mChart;
    Axis mAxis;
    double mMin;
    double mMax;
};

std::unique_ptr<IChart::ISeries> Chart::CreateSeries(Axis axis, char const *name)
{
    if (name != nullptr)
    {
        if (axis == Axis::Left or axis == Axis::Right)
            return std::unique_ptr<ISeries>(new Series(*this, axis, name));
    }
    return nullptr;
}

void Chart::AddSeries(Axis axis, SeriesData series)
{
    mSeries[axis - 1].push_back(std::move(series));
}

void Chart::SetCoordinators()
{
    if (mValues[Axis::Horizontal] == nullptr)
    {
        mValues[Axis::Horizontal].reset(new QGraphicsSimpleTextItem(chart()));
        for (Axis axis : {Axis::Left, Axis::Right})
        {
            if (not mSeries[axis - 1].empty())
                mValues[axis].reset(new QGraphicsSimpleTextItem(chart()));
        }
    }

    qreal middle = chart()->size().width() / 2;
    qreal height = chart()->size().height() - 20;

    if (mValues[Axis::Left] != nullptr and mValues[Axis::Right] != nullptr)
    {
        mValues[Axis::Horizontal]->setPos(middle - 320, height);
        mValues[Axis::Left]->setPos(middle, height);
        mValues[Axis::Right]->setPos(middle + 160, height);

    }
    else if (mValues[Axis::Left] != nullptr)
    {
        mValues[Axis::Horizontal]->setPos(middle - 320, height);
        mValues[Axis::Left]->setPos(middle, height);
    }
    else if (mValues[Axis::Right] != nullptr)
    {
        mValues[Axis::Horizontal]->setPos(middle - 320, height);
        mValues[Axis::Right]->setPos(middle, height);
    }
    else
    {
        mValues[Axis::Horizontal].reset();
    }

    for (Axis axis : {Axis::Horizontal, Axis::Left, Axis::Right})
    {
        if (mValues[axis] != nullptr)
            mValues[axis]->setText(sValuePrefix[axis]);
    }
}

void Chart::resizeEvent(QResizeEvent *event)
{
    if (scene())
    {
        scene()->setSceneRect(QRect(QPoint(0, 0), event->size()));
        chart()->resize(event->size());
        SetCoordinators();
    }
    QChartView::resizeEvent(event);
}

void Chart::mousePressEvent(QMouseEvent *event)
{
    setRubberBand(QChartView::RectangleRubberBand);

    if (event->button() == Qt::LeftButton or event->button() == Qt::RightButton)
        mActions.push_back(Action{false, event->button(), event->pos(), event->pos()});
    QChartView::mousePressEvent(event);
}

void Chart::mouseReleaseEvent(QMouseEvent *event)
{
    setRubberBand(QChartView::NoRubberBand);

    if (not mActions.empty() and not mActions.back().mComplete)
    {
        Action &action = mActions.back();
        action.mComplete = true;

        if (action.mButton == Qt::LeftButton)
        {
            action.mEnd = event->pos();
            auto bx = action.mBegin.x(), by = action.mBegin.y(), ex = action.mEnd.x(), ey = action.mEnd.y();
            chart()->zoomIn(QRectF(std::min(bx, ex), std::min(by, ey), abs(bx - ex), abs(by - ey)));
        }
        else
        {
            QPoint pos = event->pos();
            chart()->scroll(action.mEnd.x() - pos.x(), pos.y() - action.mEnd.y());
            action.mEnd = pos;
        }
    }
    QChartView::mouseReleaseEvent(event);
}

void Chart::mouseMoveEvent(QMouseEvent *event)
{
    QPoint const pos = event->pos();

    if (mValues[Axis::Horizontal] != nullptr and mIndexToTime)
    {
        Time time = mIndexToTime(chart()->mapToValue(pos).x());
        mValues[Axis::Horizontal]->setText(QString(sValuePrefix[Axis::Horizontal]) + ToString(time, 22u));
        for (Axis axis : {Axis::Left, Axis::Right})
        {
            if (mValues[axis] != nullptr)
            {
                qreal value = chart()->mapToValue(pos, mSeries[axis - 1][0].mSeries).y();
                int factor = value < 100 ? 1000 : (value < 1000 ? 100 : 10);
                mValues[axis]->setText(QString(sValuePrefix[axis]) + QString("%1").arg(int64_t(value * factor + factor / 2) / qreal(factor)));
            }
        }
    }

    if ((event->buttons() & Qt::RightButton) != 0 and not mActions.empty() and mActions.back().mButton == Qt::RightButton)
    {
        Action &action = mActions.back();
        chart()->scroll(action.mEnd.x() - pos.x(), pos.y() - action.mEnd.y());
        action.mEnd = pos;
    }
    QChartView::mouseMoveEvent(event);
}

void Chart::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
        return OnKeyEscape();
    case Qt::Key_Left:
        return OnHorizontalAxisScroll(1);
    case Qt::Key_Right:
        return OnHorizontalAxisScroll(-1);
    case Qt::Key_Up:
        return OnHorizontalAxisZoom(1);
    case Qt::Key_Down:
        return OnHorizontalAxisZoom(-1);
    }
    QGraphicsView::keyPressEvent(event);
}

void Chart::OnKeyEscape()
{
    if (not mActions.empty() and mActions.back().mComplete)
    {
        if (mActions.back().mButton == Qt::LeftButton)
        {
            chart()->zoomReset();
            auto it = mActions.begin();
            while (it != mActions.end() and it->mButton == Qt::RightButton)
                ++it;
            while (it + 1 < mActions.end())
            {
                Action const &action = *it++;

                if (action.mButton == Qt::LeftButton)
                {
                    auto bx = action.mBegin.x(), by = action.mBegin.y(), ex = action.mEnd.x(), ey = action.mEnd.y();
                    chart()->zoomIn(QRectF(std::min(bx, ex), std::min(by, ey), abs(bx - ex), abs(by - ey)));
                }
                else
                {
                    chart()->scroll(action.mBegin.x() - action.mEnd.x(), action.mEnd.y() - action.mBegin.y());
                }
            }
        }
        else
        {
            Action const &action = mActions.back();
            chart()->scroll(action.mEnd.x() - action.mBegin.x(), action.mBegin.y() - action.mEnd.y());
        }
        mActions.pop_back();
    }
}

QtCharts::QCategoryAxis * Chart::GetHorizontalAxis()
{
    QList<QtCharts::QAbstractAxis *> axes = chart()->axes(Qt::Horizontal);
    if (axes.size() == 1)
        return qobject_cast<QtCharts::QCategoryAxis *>(axes.first());
    return nullptr;
}

void Chart::ResetActions()
{
    if (not mActions.empty() and mActions.back().mComplete)
    {
        chart()->zoomReset();
        auto it = std::find_if(mActions.begin(), mActions.end(), [](Action const &a) noexcept { return a.mButton == Qt::LeftButton; });
        if (it != mActions.begin())
        {
            Action const &action = *--it;
            chart()->scroll(action.mEnd.x() - mActions.front().mBegin.x(), mActions.front().mBegin.y() - action.mEnd.y());
        }
        mActions.clear();
    }
}

void Chart::OnHorizontalAxisScroll(int direction)
{
    if (not mActions.empty() and not mActions.back().mComplete)
        return;

    ResetActions();

    if (mHorizontalRangeLength == std::numeric_limits<int64_t>::max())
        return;

    int64_t const length = mIndicesSegments.back() - mIndicesSegments.front();
    int64_t const rangeLength = mHorizontalRangeLength * mHorizontalRangeScaler;
    if (rangeLength < length)
    {
        int64_t scroll = std::max(0l, std::min(length - rangeLength, mHorizontalRangeScroll + direction * rangeLength));
        if (scroll != mHorizontalRangeScroll)
        {
            QtCharts::QCategoryAxis *horizontalAxis = GetHorizontalAxis();
            if (horizontalAxis != nullptr)
            {
                mHorizontalRangeScroll = int32_t(scroll);

                int64_t max = mIndicesSegments.back() - scroll;
                horizontalAxis->setMax(max);
                horizontalAxis->setMin(max - rangeLength);
            }
        }
    }
}

void Chart::OnHorizontalAxisZoom(int direction)
{
    if (not mActions.empty() and not mActions.back().mComplete)
        return;

    ResetActions();

    if (mHorizontalRangeLength == std::numeric_limits<int64_t>::max())
        return;

    int64_t const length = mIndicesSegments.back() - mIndicesSegments.front();
    if (0 < direction and length <= mHorizontalRangeScaler * mHorizontalRangeLength)
        return;

    int const scalers[] = {1, 2, 5, 10, 20, 50, 100, 200, 500, 1000};
    size_t const N = sizeof(scalers) / sizeof(scalers[0]);

    int const *it = std::find(&scalers[0], &scalers[N], int32_t(mHorizontalRangeScaler));
    if (it != &scalers[N] and &scalers[0] <= it + direction and it + direction < &scalers[N])
    {
        QtCharts::QCategoryAxis *horizontalAxis = GetHorizontalAxis();
        if (horizontalAxis != nullptr)
        {
            int32_t scaler = *(it + direction);

            int64_t rangeLength = scaler * mHorizontalRangeLength;
            if (mHorizontalRangeScroll + rangeLength <= length)
            {
                int64_t max = mIndicesSegments.back() - mHorizontalRangeScroll;
                horizontalAxis->setMax(max);
                horizontalAxis->setMin(max - rangeLength);
            }
            else
            {
                int64_t min = mIndicesSegments.front();
                int64_t max = std::min(mIndicesSegments.back(), min + rangeLength);
                horizontalAxis->setMin(min);
                horizontalAxis->setMax(max);
            }

            mHorizontalRangeScaler = int16_t(scaler);
        }
    }
}

