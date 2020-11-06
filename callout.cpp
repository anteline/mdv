#include <callout.h>
#include <chart.h>
#include <QtGui/QPainter>
#include <QtGui/QFontMetrics>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtGui/QMouseEvent>
#include <QtCharts/QChart>

Callout::Callout(Chart &chart):
    QGraphicsItem(chart.chart()),
    mChart(chart),
    mSeries(nullptr)
{
}

QRectF Callout::boundingRect() const
{
    QPointF anchor = mapFromParent(mChart.chart()->mapToPosition(mAnchor, mSeries));
    QRectF rect;
    rect.setLeft(qMin(mRect.left(), anchor.x()));
    rect.setRight(qMax(mRect.right(), anchor.x()));
    rect.setTop(qMin(mRect.top(), anchor.y()));
    rect.setBottom(qMax(mRect.bottom(), anchor.y()));
    return rect;
}

void Callout::paint(QPainter *painter, QStyleOptionGraphicsItem const *, QWidget *)
{
    QPainterPath path;
    path.addRoundedRect(mRect, 5, 5);

    QPointF anchor = mapFromParent(mChart.chart()->mapToPosition(mAnchor, mSeries));
    if (!mRect.contains(anchor))
    {
        QPointF point1, point2;

        // establish the position of the anchor point in relation to mRect
        bool above = anchor.y() <= mRect.top();
        bool aboveCenter = anchor.y() > mRect.top() && anchor.y() <= mRect.center().y();
        bool belowCenter = anchor.y() > mRect.center().y() && anchor.y() <= mRect.bottom();
        bool below = anchor.y() > mRect.bottom();

        bool onLeft = anchor.x() <= mRect.left();
        bool leftOfCenter = anchor.x() > mRect.left() && anchor.x() <= mRect.center().x();
        bool rightOfCenter = anchor.x() > mRect.center().x() && anchor.x() <= mRect.right();
        bool onRight = anchor.x() > mRect.right();

        // get the nearest mRect corner.
        qreal x = (onRight + rightOfCenter) * mRect.width();
        qreal y = (below + belowCenter) * mRect.height();
        bool cornerCase = (above && onLeft) || (above && onRight) || (below && onLeft) || (below && onRight);
        bool vertical = qAbs(anchor.x() - x) > qAbs(anchor.y() - y);

        qreal x1 = x + leftOfCenter * 10 - rightOfCenter * 20 + cornerCase * !vertical * (onLeft * 10 - onRight * 20);
        qreal y1 = y + aboveCenter * 10 - belowCenter * 20 + cornerCase * vertical * (above * 10 - below * 20);;
        point1.setX(x1);
        point1.setY(y1);

        qreal x2 = x + leftOfCenter * 20 - rightOfCenter * 10 + cornerCase * !vertical * (onLeft * 20 - onRight * 10);;
        qreal y2 = y + aboveCenter * 20 - belowCenter * 10 + cornerCase * vertical * (above * 20 - below * 10);;
        point2.setX(x2);
        point2.setY(y2);

        path.moveTo(point1);
        path.lineTo(anchor);
        path.lineTo(point2);
        path = path.simplified();
    }

    painter->setBrush(QColor(255, 255, 255));
    painter->drawPath(path);
    painter->drawText(mTextRect, mText);
}

void Callout::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    event->setAccepted(true);
    if (event->buttons() == Qt::RightButton)
        mChart.RemoveCallout(*this);
}

void Callout::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        setPos(mapToParent(event->pos() - event->buttonDownPos(Qt::LeftButton)));
        event->setAccepted(true);
    }
    else
    {
        event->setAccepted(false);
    }
}

void Callout::SetText(std::string const &text)
{
    mText = QString::fromStdString(text);
    QFontMetrics metrics(mFont);
    mTextRect = metrics.boundingRect(QRect(0, 0, 150, 150), Qt::AlignLeft, mText);
    mTextRect.translate(5, 5);
    prepareGeometryChange();
    mRect = mTextRect.adjusted(-5, -5, 5, 5);
}

void Callout::SetAnchor(QPointF point, QtCharts::QLineSeries *series)
{
    mAnchor = point;
    mSeries = series;
}

void Callout::UpdateGeometry()
{
    prepareGeometryChange();
    setPos(mChart.chart()->mapToPosition(mAnchor, mSeries) + QPoint(10, -50));
}

