/* ---------------------------------------------------------------------------
** This software is furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** File Name: dataplot.cpp
**
** plots the data on a graph - aggralinx software
**
** Author: Michael W. Hoag
** Copyright Michael W. Hoag 2016
** Email: mike@ndtjames.com
** -------------------------------------------------------------------------*/

/*
 * Window always stays at the back in Linux, check same in Windows?
*/

#include "dataplot.h"

/*
 * Blind Constructor with no datapoints
 * Mostly Sets Parameters
*/

DataPlot::DataPlot( QWidget* )
{
    createClasses();
    SetPlotParameters();
    SetGridParameters();
    SetCurveParameters();
    SetRCurveParameters();

    plotDataPoints<< QPointF (0.0,0.0);
}

/*
 *Destructor makes sure everything is deleted
*/

DataPlot::~DataPlot()
{
//delete symbol creates a problem.
    delete Curve;
    delete Grid;
}

/*
 * creates the new classes (allocates memory)
*/

void DataPlot::createClasses()
{
    Curve = new QwtPlotCurve;
    Grid = new QwtPlotGrid;
    Symbol = new QwtSymbol( QwtSymbol::Ellipse,
                QBrush( Qt::red ),
                QPen( Qt::red, 2 ),
                QSize( 8, 8 ) );
    rCurve = new QwtPlotCurve;
    rSymbol = new QwtSymbol(QwtSymbol::Ellipse,
                            QBrush( Qt::red ),
                            QPen( Qt::red, 2 ),
                            QSize( 8, 8 ));
}

/*
*loads the datapoint vector with data
*/

void DataPlot::createPoints(const QString& rawdata)
{
    QString line = "";
    QStringList datalines = rawdata.split("\n", QString::SkipEmptyParts);
    QStringList noheader = datalines.filter(tr("Direct"));


    plotDataPoints.clear();
    foreach (line, noheader) {
        if(!loadPlotDataPoints(line)) return;
    }
    displayGraph(plotDataPoints);
}

/*
 * Draws the linear regression line
 */

void DataPlot::createRegLine(const QVector<QPointF>& in)
{
    QVector<QPointF> regLineData;
    Regression* analysis = new Regression();

    for(qint64 i = 0; i < in.size(); ++i){
        exp_data loaddata;
        loaddata.x = in[i].x();
        loaddata.y = in[i].y();
        analysis->addData(loaddata);
    }

    regLineData << QPointF(0.0, analysis->offset())
                << QPointF(maxAD(), analysis->slope()*maxAD()
                            + analysis->offset());
    rCurve->setSamples( regLineData );
    rCurve->attach( this );
/* Text output of lines parameters */
    QString p = QString("Correlation Line:") + '\n' +
                QString("Gain: ") + QString::number(analysis->slope(),'f', 4.4) + '\n' +
                QString("Offset: ") + QString::number(analysis->offset(),'f', 4.3)+ '\n' +
                QString("R Squared: ")+ QString::number(analysis->rsqu(),'f',4.4);

    displayRegParameters(p);
    rCurve->setLegendAttribute(QwtPlotCurve::LegendShowSymbol, false);
    delete analysis;
}

/*
 * Shows the graph
 * Sets some parameters that are data dependant
*/

void DataPlot::displayGraph(const QVector<QPointF>& points)
{
    setAxisScale(QwtPlot::yLeft, minAxisScale(), maxY(points));
    setAxisScale(QwtPlot::xBottom, minAxisScale(), maxX(points));
    Curve->setSamples( points );
    Curve->attach(this);

    createRegLine(plotDataPoints);

    resize( 1200, 800 );
    replot();
    show();
    setFocus();
    raise();
}

/*
 * set parameters for regression parameter text
*/
void DataPlot::displayRegParameters(const QString &in){

    QwtPlotTextLabel* text_label = new QwtPlotTextLabel;
    QwtText qwt_text(in);
    QFont font = qwt_text.font();
    font.setPointSize(14);
    qwt_text.setFont(font);
    qwt_text.setColor(Qt::black);
    qwt_text.setRenderFlags( Qt::AlignLeft | Qt::AlignTop );

    text_label->setText(qwt_text);
    text_label->attach( this );
    text_label->show();
}

/*
 * loads datapoints from string
*/
bool DataPlot::loadPlotDataPoints(const QString& line)
{
    bool success = false, ok = false, second = false;
    double dr = 0.0;
    double dm = 0.0;
    QString buffer;
    QStringList list = line.split(" ");

    foreach(buffer, list){
        buffer.toDouble( &ok );
        if(ok == true && second == true ){//found second valid double
            QTextStream bufferstream(& buffer); //strange bug makes two versions of bufferstream
            bufferstream >> dm;
            success = true;
            break;
        }
        if(ok == true && second == false){//found the first valid double
            QTextStream bufferstream(& buffer);  //strange bug makes two versions of bufferstream
            second = true;
            bufferstream >> dr;
            bufferstream.flush();
        }
    }

    if(ok == false){
        QMessageBox::information(this, "Aggralinx", tr("Bad Data File"));
    }

    if(second == false){
        QMessageBox::information(this,"Aggralinx",tr("Data Not Entered"));
    }

    plotDataPoints << QPointF(dr,dm);
    return(success);
}

/*
 * a constructor function to set general chart parameters
 * */
void DataPlot::SetPlotParameters()
{
    setTitle(tr("Aggralinx Regression"));
    setAxisTitle( QwtPlot::xBottom, "Aggrameter Value");
    setAxisTitle( QwtPlot::yLeft, "Moisture Value");
    setCanvasBackground( Qt::white );
    insertLegend( new QwtLegend() );

}
/*
 * a constructor function to set general curve parameters
 */
void DataPlot::SetCurveParameters()
{
    Curve->setTitle( "Data Points" );
    Curve->setStyle(QwtPlotCurve::NoCurve);
    Curve->setRenderHint( QwtPlotItem::RenderAntialiased, true );
    Curve->setSymbol( Symbol );
}

/*
 * a constructor function to set general grid parameters
 */

void DataPlot::SetGridParameters()
{
    Grid->attach(this);
}

/*
 * a constructor function to set general regression
 * parameters
 */

void DataPlot::SetRCurveParameters()
{
    rCurve->setStyle(QwtPlotCurve::Lines);
    rCurve->setRenderHint( QwtPlotItem::RenderAntialiased, true );
}
/*
 * to set scale of chart
 */

qreal DataPlot::maxY(const QVector<QPointF> &in)
{
    qreal rvalue = 0.0;
    qreal test = 0.0;
    QVector<QPointF> buffer = in;

    for(int i = 0; i<in.size();++i){
        test = buffer[i].ry();
        rvalue = qMax( test, rvalue);
    }
    return(rvalue);
}

/*
 * to set scale of chart
 */

qreal DataPlot::maxX(const QVector<QPointF> &in)
{
    qreal rvalue = 0.0;
    qreal test = 0.0;
    QVector<QPointF> buffer = in;

    for(int i = 0; i<in.size();++i){
        test = buffer[i].rx();
        rvalue = qMax( test, rvalue);
    }
    return(rvalue);
}
