#include <chart.h>
#include <callout.h>
#include <QPen>
#include <QtCharts/QCategoryAxis>
#include <QtCharts/QChart>
#include <QtGui/QMouseEvent>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsTextItem>
#include <algorithm>
#include <limits>
#include <math.h>


static QString ToString(Time time, size_t length)
{
    std::array<char, 32> str = time.ToString();
    if (length < sizeof(str))
        str[length] = char(0);
    return QString(str.data());
}

class Series : public IChart::ISeries
{
public:
    explicit Series(Chart &chart, Fixpoint centre, char const *name)
    :   mSeries(new QtCharts::QLineSeries),
        mChart(chart),
        mCentre(centre),
        mMin(std::numeric_limits<double>::max()),
        mMax(-std::numeric_limits<double>::max()),
        mName(name)
    {
        mSeries->setName(name);
    }

    virtual void Commit() override final
    {
        if (mSeries != nullptr)
            mChart.AddSeries(mSeries.release(), std::move(mName), mCentre, mMin, mMax);
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
    Chart   &mChart;
    Fixpoint mCentre;
    double   mMin;
    double   mMax;
    std::string mName;
};

Chart::Chart()
:   QtCharts::QChartView(new QtCharts::QChart, nullptr),
    mHorizontalRangeLength(std::numeric_limits<int64_t>::max()),
    mHorizontalRangeScroll(0),
    mHorizontalRangeScaler(1),
    mVisible(false),
    mVLine(nullptr),
    mCallout(nullptr)
{
    chart()->setMinimumSize(640, 480);
    chart()->legend()->setVisible(true);
    chart()->legend()->setAlignment(Qt::AlignBottom);

    setRenderHint(QPainter::Antialiasing);

    mVLine = new QGraphicsLineItem();
    mVLine->setPen(QPen(Qt::PenStyle::DotLine));
    mVLine->setVisible(true);
    scene()->addItem(mVLine);
}

Chart::~Chart()
{
}

void Chart::RemoveCallout(Callout &callout)
{
    if (mCallout == &callout)
        mCallout = nullptr;

    mCallouts.erase(std::find(mCallouts.begin(), mCallouts.end(), &callout));
    delete &callout;
}

bool Chart::Show()
{
    if (mVisible)
        return true;

    if (mIndexToTime and (not(mSeries[0].empty()) or not(mSeries[1].empty())))
    {
        QtCharts::QCategoryAxis *horizontalAxis = new QtCharts::QCategoryAxis;
        horizontalAxis->setLabelsPosition(QtCharts::QCategoryAxis::AxisLabelsPositionOnValue);

        for (size_t i = 0; i < mIndicesSegments.size(); ++i)
        {
            int64_t index = mIndicesSegments[i];
            QString label = ToString(mIndexToTime(index), 16u);
            horizontalAxis->append(label, index);
        }

        horizontalAxis->setMin(std::max(mIndicesSegments.front(), mIndicesSegments.back() - mHorizontalRangeLength));
        horizontalAxis->setMax(mIndicesSegments.back());
        chart()->addAxis(horizontalAxis, Qt::AlignBottom);

        AddCentreFixedData(horizontalAxis, mSeries[1]);
        AddCentreUnfixedData(horizontalAxis, mSeries[0]);

        show();
        mVisible = true;
        return true;
    }
    return false;
}

SeriesData::SeriesData(Chart &chart, QtCharts::QLineSeries *series, std::string name, double centre, double min, double max)
:   mChart(&chart),
    mSeries(series),
    mAxis(nullptr),
    mName(std::move(name)),
    mCentre(centre),
    mMin(min),
    mMax(max)
{
}

void SeriesData::SetRange()
{
    if (mAxis == nullptr)
        return;

    double min = mMin, max = mMax;
    if (mCentre * 2 < min + max)
    {
        min = mCentre * 2 - max;
    }
    else if (min + max < mCentre * 2)
    {
        max = mCentre * 2 - min;
    }
    mAxis->setMin(min);
    mAxis->setMax(max);
}

void Chart::AddCentreFixedData(QtCharts::QCategoryAxis *horizontalAxis, std::vector<std::unique_ptr<SeriesData>> const &data)
{
    if (data.empty())
        return;

    for (std::unique_ptr<SeriesData> const &seriesData : data)
    {
        seriesData->mAxis = new QtCharts::QValueAxis;
        chart()->addAxis(seriesData->mAxis, Qt::AlignLeft);

        seriesData->mSeries->attachAxis(horizontalAxis);
        seriesData->mSeries->attachAxis(seriesData->mAxis);

        seriesData->SetRange();
    }
}

void Chart::AddCentreUnfixedData(QtCharts::QCategoryAxis *horizontalAxis, std::vector<std::unique_ptr<SeriesData>> const &data)
{
    if (data.empty())
        return;

    QtCharts::QValueAxis *verticalAxis = new QtCharts::QValueAxis;
    chart()->addAxis(verticalAxis, Qt::AlignRight);

    double min = data.front()->mMin, max = data.front()->mMax;
    for (std::unique_ptr<SeriesData> const &seriesData : data)
    {
        seriesData->mSeries->attachAxis(horizontalAxis);
        seriesData->mSeries->attachAxis(verticalAxis);

        min = std::min(min, seriesData->mMin);
        max = std::max(max, seriesData->mMax);
    }

    verticalAxis->setMin(min);
    verticalAxis->setMax(max);
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

std::unique_ptr<IChart::ISeries> Chart::CreateSeries(Fixpoint axisCentre, char const *name)
{
    if (name != nullptr)
        return std::unique_ptr<ISeries>(new Series(*this, axisCentre, name));
    return nullptr;
}

void Chart::AddSeries(QtCharts::QLineSeries *series, std::string name, Fixpoint centre, double min, double max)
{
    bool fixedCentre = (centre != Fixpoint::Min());
    mSeries[fixedCentre].push_back(std::unique_ptr<SeriesData>(new SeriesData(*this, series, std::move(name), double(centre), min, max)));
    chart()->addSeries(series);

    connect(series, &QtCharts::QLineSeries::hovered, mSeries[fixedCentre].back().get(), &SeriesData::AddCallout);
}

void Chart::KeepCallout()
{
    if (mCallout != nullptr)
        mCallouts.push_back(mCallout);
    mCallout = new Callout(*this);
}

void SeriesData::AddCallout(QPointF pos, bool state)
{
    mChart->AddCallout(*this, pos, state);
}

std::string SeriesData::GetText(Time time, double value) const
{
    char buff[mName.length() + 64];
    snprintf(buff, sizeof(buff), "Time=%.12s \n%s=%.4f ", time.ToString().data() + 11, mName.data(), value);
    return std::string(buff);
}

void Chart::AddCallout(SeriesData &series, QPointF pos, bool state)
{
    if (mCallout == nullptr)
        mCallout = new Callout(*this);

    if (state)
    {
        mCallout->SetText(series.GetText(mIndexToTime(pos.x()), pos.y()));
        mCallout->SetAnchor(pos, series.mSeries);
        mCallout->setZValue(11);
        mCallout->UpdateGeometry();
        mCallout->show();
    }
    else
    {
        mCallout->hide();
    }
}

void Chart::UpdateCalloutsGeometry()
{
    for (Callout *callout : mCallouts)
        callout->UpdateGeometry();
}

void Chart::resizeEvent(QResizeEvent *event)
{
    if (scene())
    {
        scene()->setSceneRect(QRect(QPoint(0, 0), event->size()));
        chart()->resize(event->size());

        UpdateCalloutsGeometry();
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

        for (std::unique_ptr<SeriesData> const &seriesData : mSeries[1])
            seriesData->SetRange();
    }
    QChartView::mouseReleaseEvent(event);
}

void Chart::mouseMoveEvent(QMouseEvent *event)
{
    QPoint const pos = event->pos();

    if ((event->buttons() & Qt::RightButton) != 0 and not mActions.empty() and mActions.back().mButton == Qt::RightButton)
    {
        Action &action = mActions.back();
        chart()->scroll(action.mEnd.x() - pos.x(), pos.y() - action.mEnd.y());
        action.mEnd = pos;

        for (std::unique_ptr<SeriesData> const &seriesData : mSeries[1])
            seriesData->SetRange();
    }

    if (mVLine->isVisible() and scene() != nullptr)
    {
        QRectF rect = chart()->plotArea();
        mVLine->setLine(pos.x(), rect.top(), pos.x(), rect.bottom());
    }
    QChartView::mouseMoveEvent(event);
}

void Chart::OnToggleVerticalLine()
{
    mVLine->setVisible(not mVLine->isVisible());
}

void Chart::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
        return OnKeyEscape();
    case Qt::Key_F3:
        return OnToggleVerticalLine();
    case Qt::Key_F:
        return KeepCallout();
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
    if (mActions.empty())
    {
        // redraw all components
        mVisible = false;

        for (QtCharts::QAbstractSeries *series : chart()->series())
        {
            for (QtCharts::QAbstractAxis *axis : series->attachedAxes())
                series->detachAxis(axis);
        }
        for (QtCharts::QAbstractAxis *axis : chart()->axes())
            chart()->removeAxis(axis);

        Show();
    }
    else if (mActions.back().mComplete)
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

