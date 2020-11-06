#ifndef CALLOUT_H
#define CALLOUT_H
#include <QtCharts/QChartGlobal>
#include <QtCharts/QLineSeries>
#include <QtGui/QFont>
#include <QtWidgets/QGraphicsItem>
#include <string>

class Chart;
class QGraphicsSceneMouseEvent;

namespace QtCharts {
class QChart;
}

class Callout : public QGraphicsItem
{
public:
    Callout(Chart &parent);

    void SetText(std::string const &text);
    void SetAnchor(QPointF point, QtCharts::QLineSeries *series);
    void UpdateGeometry();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

    virtual QRectF boundingRect() const override;
    virtual void paint(QPainter *painter, QStyleOptionGraphicsItem const *, QWidget *) override;

private:
    QString mText;
    QRectF mTextRect;
    QRectF mRect;
    QPointF mAnchor;
    QFont mFont;
    Chart &mChart;
    QtCharts::QLineSeries *mSeries;
};

#endif // CALLOUT_H
