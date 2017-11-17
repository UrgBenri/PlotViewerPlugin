/*
	This file is part of the UrgBenri application.

	Copyright (c) 2016 Mehrez Kristou.
	All rights reserved.

	Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

	3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	Please contact kristou@hokuyo-aut.jp for more details.

*/

#include "PlotViewerPlugin.h"
#include "ui_PlotViewerPlugin.h"

#include <algorithm>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QShortcut>
#include <QDesktopWidget>

#define ECHO_COUNT 3

PlotViewerPlugin::PlotViewerPlugin(QWidget *parent)
    : ViewerPluginInterface(parent)
    , ui(new Ui::PlotViewerPlugin)
    , currentScan(0)
    , refreshRate(100)
    , refreshTimerAvg(40, 25)
    , m_settings(Q_NULLPTR)
{
    initialize();

//    ui->orthoDrawWidget->addLine(QLineF(0, 0, 1000, 1000), 5, QColor(Qt::red));
//    ui->orthoDrawWidget->addCircle(QPointF(1000, -1000), 500, QColor(Qt::green));
}

void PlotViewerPlugin::initSettings()
{
    for(int i = 0; i < 5; ++i){
        setDistanceLineColor(i, QColor(Qt::green));
        setDistancePointColor(i,  QColor(Qt::red));
        setDistanceShow(i, true);
        setIntensityLineColor(i, QColor(Qt::magenta));
        setIntensityPointColor(i, QColor(Qt::magenta));
        setIntensityShow(i, true);
    }

    showDistance(true);
    ShowIntensity(true);
    intensityOnTop(true);
    setRotationOffset(0);

    setDrawingMode(PlotViewerPlugin::Lines);

    setDistanceLineSize(1);
    setDistancePointSize(1);

    setIntensityLineSize(1);
    setIntensityPointSize(1);
    setIntensityRatio(100);

    setRefreshRate(0);
    setDrawPeriod(25);
}

void PlotViewerPlugin::reszeValuesTable()
{
    ui->stepInfo->setText("");
}

PlotViewerPlugin::~PlotViewerPlugin()
{
    if(m_settings) delete m_settings;
    delete ui;
}

void PlotViewerPlugin::setIntensityRatio(int val)
{
    double ratio = (double)val / 100.0;
    ui->orthoDrawWidget->setIntensityRatio(ratio);
}

void PlotViewerPlugin::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            if(ui) ui->retranslateUi(this);
            break;
        default:
            break;
    }
}

void PlotViewerPlugin::hideEvent(QHideEvent *)
{
    m_settings->close();
}

void PlotViewerPlugin::addMeasurementData(const QString &id, const PluginDataStructure &data)
{
    //TODO: investigate this, memory leak!!!!
    localData = data;
    localData.ranges = invertData(localData.ranges, invertedView);
    localData.levels = invertData(localData.levels, invertedView);


    if (refreshTimer.elapsed() > refreshRate) {
        refreshTimerAvg.push_back(refreshTimer.restart());
        refresh();
    }
}

void PlotViewerPlugin::setReceptionRate(int rate)
{
    ui->orthoDrawWidget->setUpperRightMessage(tr("%1ms").arg(rate, 3, 10, QChar(' ')));
}

void PlotViewerPlugin::setRotationOffset(int rotation)
{
    ui->orthoDrawWidget->setRotationOffset(rotation);
}

//void PlotWidget::setRangeSensor(const RangeSensor* sensor)
//{
//    localSensor = sensor;
//    ui->orthoDrawWidget->setConvertor(localSensor);
//}

void PlotViewerPlugin::orthoMouseMove(bool state, long x, long y, int step)
{
    if (state) {
        emit mousePositionChanged(x, y, step);
    }
}

void PlotViewerPlugin::orthoMouseClicked(bool state, long x, long y, int step)
{
    if (state) {
        emit mousePositionClicked(x, y, step);
        setSelectedStep(step);
        emit selectedStepChanged(step);
    }
}

void PlotViewerPlugin::refresh()
{
    ui->orthoDrawWidget->addMeasurementData(localData);
    ui->orthoDrawWidget->setLowerRightMessage(tr("Refresh rate: ") + QString::number(1000 / refreshTimerAvg.average()) + " fps");
    updateValuesTable(selectedStep);
}

void PlotViewerPlugin::orthoSnapshot()
{
    QPixmap pixmap = ui->orthoDrawWidget->renderPixmap(0, 0, true);
    QString defaulName = QString("/") +
            QDateTime::currentDateTime().toString("yyyy_MM_dd_HH_mm_ss_zzz") +
            ".png";
    QString filename = QFileDialog::getSaveFileName(
                this,
                tr("Save snapshot"),
                QDir::currentPath() + defaulName,
                tr("PNG file (*.png)"));
    if (!filename.isNull()) {
        QFileInfo fi(filename);
        QDir::setCurrent(fi.absolutePath());

        pixmap.save(filename);
    }
}

void PlotViewerPlugin::setSelectedStep(int step){
    selectedStep = step;
    ui->orthoDrawWidget->setSelectedStep(step);
    updateValuesTable(selectedStep);
}

void PlotViewerPlugin::onLoad(PluginManagerInterface *manager)
{

}

void PlotViewerPlugin::onUnload()
{

}

void PlotViewerPlugin::toggleGrid()
{
    ui->orthoDrawWidget->showAxis(!ui->orthoDrawWidget->isAxisShowed());
}

void PlotViewerPlugin::updateValuesTable(int step)
{
    QString stepInfo = tr("Time: <b>%1</b>")
            .arg(QString::number(localData.timestamp), 8, '0');
    if(localData.converter){
        int fdata = localData.converter.data()->step2Index(step);
        if (fdata >= 0 && fdata < localData.ranges.size()) {
            stepInfo += "  | " + tr("Step: <b>%1</b>")
                    .arg(QString::number(step), 4, '0');

            for(int i = 0; i < ECHO_COUNT; ++i){
                if(localData.ranges[fdata].size() > i){
                    stepInfo += ", " + tr("Echo%1: <b>%2</b>")
                            .arg(i)
                            .arg(QString::number(localData.ranges[fdata][i]), 5, '0');
                }
            }
        }
    }

    ui->stepInfo->setText(stepInfo);
}

void PlotViewerPlugin::setupConnections()
{
    connect(ui->orthoDrawWidget, &UrgDrawWidget::positionUpdated,
            this, &PlotViewerPlugin::orthoMouseMove);
    connect(ui->orthoDrawWidget, &UrgDrawWidget::positionClicked,
            this, &PlotViewerPlugin::orthoMouseClicked);

    connect(ui->orthoZoominButton, &QAbstractButton::clicked,
            ui->orthoDrawWidget, &UrgDrawWidget::largerZoom);
    connect(ui->orthoZzoomoutButton, &QAbstractButton::clicked,
            ui->orthoDrawWidget, &UrgDrawWidget::smallerZoom);
    connect(ui->orthoZoomfitButton, &QAbstractButton::clicked,
            ui->orthoDrawWidget, &UrgDrawWidget::initializeView);

    connect(ui->settingsButton, &QToolButton::clicked,
            this, [=](){

        m_settings->show();
        m_settings->raise();

        int screenBottom = QApplication::desktop()->screenGeometry(QApplication::desktop()->screenNumber(m_settings)).bottom();

        QPoint position = ui->settingsButton->mapToGlobal(QPoint(0, ui->settingsButton->height()));
        m_settings->move(position);

        QRect settingsRect = m_settings->geometry();
        settingsRect.setHeight(680);
        if(settingsRect.bottom() > screenBottom ){
            int diff = settingsRect.bottom() - screenBottom;
            settingsRect.setHeight(settingsRect.height() - diff);
        }
        m_settings->setGeometry(settingsRect);

    });
    connect(ui->orthoClear, &QAbstractButton::clicked,
            ui->orthoDrawWidget, &UrgDrawWidget::clear);

    connect(ui->orthoRefreshAction, &QAbstractButton::clicked,
            this, &PlotViewerPlugin::refresh);

    connect(ui->orthoSnapshot, &QAbstractButton::clicked,
            this, &PlotViewerPlugin::orthoSnapshot);
}

void PlotViewerPlugin::setupShortcuts()
{
    //    (void) new QShortcut(QKeySequence(Qt::ALT + Qt::Key_O), this, SLOT(showOrthoView()));
    //    (void) new QShortcut(QKeySequence(Qt::ALT + Qt::Key_T), this, SLOT(showTableView()));
    //    (void) new QShortcut(QKeySequence(Qt::ALT + Qt::Key_G), this, SLOT(toggleGrid()));
}

QVector<QVector<long > > PlotViewerPlugin::invertData(const QVector<QVector<long > > &steps, bool toInvert)
{
    QVector<QVector<long > > result = steps;
    if (toInvert && !result.isEmpty()) {
        std::reverse(result.begin(), result.end());
    }
    return result;
}

void PlotViewerPlugin::clear()
{
    localData.ranges.clear();
    localData.levels.clear();
    ui->orthoDrawWidget->clear();
    ui->stepInfo->setText("");
}

void PlotViewerPlugin::setDistanceLineColor(int echo, QColor color)
{
    ui->orthoDrawWidget->setRangeLineColor(echo, color);
}

void PlotViewerPlugin::setDistancePointColor(int echo, QColor color)
{
    ui->orthoDrawWidget->setRangePointColor(echo, color);
}

void PlotViewerPlugin::setDistanceShow(int echo, bool state)
{
    ui->orthoDrawWidget->setRangeShow(echo, state);
}

void PlotViewerPlugin::setDistanceLineSize(int size)
{
    ui->orthoDrawWidget->setRangeLineSize(size);
}

void PlotViewerPlugin::setDistancePointSize(int size)
{
    ui->orthoDrawWidget->setRangePointSize(size);
}

void PlotViewerPlugin::setIntensityLineColor(int echo, QColor color)
{
    ui->orthoDrawWidget->setLevelLineColor(echo, color);
}

void PlotViewerPlugin::setIntensityPointColor(int echo, QColor color)
{
    ui->orthoDrawWidget->setLevelPointColor(echo, color);
}

void PlotViewerPlugin::setIntensityShow(int echo, bool state)
{
    ui->orthoDrawWidget->setIntensityShow(echo, state);
}

void PlotViewerPlugin::setIntensityLineSize(int size)
{
    ui->orthoDrawWidget->setLevelLineSize(size);
}

void PlotViewerPlugin::setIntensityPointSize(int size)
{
    ui->orthoDrawWidget->setLevelPointSize(size);
}

void PlotViewerPlugin::setEchoSelection(int index)
{
    ui->orthoDrawWidget->setEchoSelection(index);
}

void PlotViewerPlugin::setDrawingMode(DrawMode mode)
{
    switch (mode) {
        case Points: {
            ui->orthoDrawWidget->setDrawMode(UrgDrawWidget::Points);
        }
            break;
        case Lines: {
            ui->orthoDrawWidget->setDrawMode(UrgDrawWidget::Lines);
        }
            break;
        case Polygon: {
            ui->orthoDrawWidget->setDrawMode(UrgDrawWidget::Polygon);
        }
            break;
        default: {
            ui->orthoDrawWidget->setDrawMode(UrgDrawWidget::Lines);
        }
    }
}

void PlotViewerPlugin::setDrawPeriod(size_t msec)
{
    ui->orthoDrawWidget->setDrawPeriod(msec);
}
void PlotViewerPlugin::recenter()
{
    ui->orthoDrawWidget->setViewCenter(0.0, 0.0);
    refresh();
}

void PlotViewerPlugin::resetView()
{
    ui->orthoDrawWidget->initializeView();
    currentScan = 0;
}

void PlotViewerPlugin::saveState(QSettings &settings)
{
    m_settings->saveState(settings);
}

void PlotViewerPlugin::restoreState(QSettings &settings)
{
    m_settings->restoreState(settings);
}

void PlotViewerPlugin::loadTranslator(const QString &locale)
{
    qApp->removeTranslator(&m_translator);
    if (m_translator.load(QString("plugin.%1").arg(locale), ":/PlotViewerPlugin")) {
        qApp->installTranslator(&m_translator);
    }
}

void PlotViewerPlugin::initialize()
{
    ui->setupUi(this);

    m_settings = new PlotSettingsWidget();
    m_settings->setWindowFlags(Qt::Popup);
    ui->orthoDrawWidget->initializeView();

    setDistanceLineColor(0, Qt::green);
    setDistancePointColor(0, Qt::red);
    setIntensityLineColor(0, Qt::magenta);
    setIntensityPointColor(0, Qt::red);

    setDrawingMode(Lines);

    invertedView = false;

    setupConnections();
    setupShortcuts();

    selectedStep = -1;

    refreshTimerAvg.setAverageValue(refreshRate);
    refreshTimer.start();

    reszeValuesTable();

    ui->orthoRefreshAction->setVisible(false);

    connect(ui->orthoDrawWidget, &UrgDrawWidget::error,
            this, &PlotViewerPlugin::error);
    connect(ui->orthoDrawWidget, &UrgDrawWidget::information,
            this, &PlotViewerPlugin::information);
    connect(ui->orthoDrawWidget, &UrgDrawWidget::warning,
            this, &PlotViewerPlugin::warning);


    initSettings();

    connect(ui->snapLastButton, &QAbstractButton::clicked,
            ui->orthoDrawWidget, &UrgDrawWidget::snapLast);
    connect(ui->snapClearButton, &QAbstractButton::clicked,
            ui->orthoDrawWidget, &UrgDrawWidget::snapClear);
    m_settings->setTarget(this);
}

void PlotViewerPlugin::showDistance(bool state)
{
    ui->orthoDrawWidget->setShowRange(state);
}

void PlotViewerPlugin::ShowIntensity(bool state)
{
    ui->orthoDrawWidget->setShowLevel(state);
}

void PlotViewerPlugin::intensityOnTop(bool state)
{
    ui->orthoDrawWidget->setLevelFirst(state);
}

