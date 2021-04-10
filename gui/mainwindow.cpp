#include <QString>
#include <QFileDialog>
#include <QDir>
#include <QMap>
#include <QDebug>
#include <QGraphicsScene>
#include <QToolButton>
#include <QFormLayout>
#include <QSettings>

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "dabtables.h"
#include "radiocontrol.h"
#include "bandscandialog.h"

#define USE_SLS_FRAME 1

const QString appName("AbracaDABra");
const QStringList syncLevelLabels = {"No signal", "Signal found", "Sync"};
const QStringList syncLevelTooltip = {"DAB signal not detected<br>Looking for signal...",
                                      "Found DAB signal,<br>trying to synchronize...",
                                      "Synchronized to DAB signal"};
const QStringList snrProgressStylesheet = {
    QString::fromUtf8("QProgressBar::chunk {background-color: #ff4b4b; }"),  // red
    QString::fromUtf8("QProgressBar::chunk {background-color: #ffb527; }"),  // yellow
    QString::fromUtf8("QProgressBar::chunk {background-color: #81bc00; }")   // green
};

enum class SNR10Threhold
{
    SNR_BAD = 60,
    SNR_GOOD = 90
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    SId.value = 0;
    //qDebug() << Q_FUNC_INFO << QThread::currentThreadId();
    dlDecoder = new DLDecoder();
    motDecoder = new MOTDecoder();

    ui->setupUi(this);

    // set UI
    setWindowTitle("Abraca DAB Radio");

    setupDialog = new SetupDialog(this);
    connect(setupDialog, &SetupDialog::inputDeviceChanged, this, &MainWindow::changeInputDevice);
    connect(setupDialog, &SetupDialog::fileLoopingEnabled, this, &MainWindow::enableFileLooping);
    connect(setupDialog, &SetupDialog::rawFileStop, this, &MainWindow::onRawFileStop);

    // status bar
    QWidget * widget = new QWidget();
    timeLabel = new QLabel("");
    timeLabel->setToolTip("DAB time");

    syncLabel = new QLabel();
    syncLabel->setAlignment(Qt::AlignRight);

    snrProgress = new QProgressBar();
    snrProgress->setMaximum(30*10);
    snrProgress->setMinimum(0);
    snrProgress->setFixedWidth(80);
    snrProgress->setFixedHeight(syncLabel->fontInfo().pixelSize());
    snrProgress->setTextVisible(false);
    snrProgress->setToolTip(QString("DAB signal SNR"));

    snrLabel = new QLabel();
#ifdef __APPLE__
    int width = snrLabel->fontMetrics().boundingRect("100.0 dB").width();
    snrLabel->setFixedWidth(width);
#endif
    snrLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    snrLabel->setToolTip(QString("DAB signal SNR"));

    QHBoxLayout * signalQualityLayout = new QHBoxLayout();
    signalQualityLayout->addWidget(syncLabel);
    signalQualityLayout->addWidget(snrProgress);
    signalQualityLayout->setStretch(0, 100);
    signalQualityLayout->setAlignment(syncLabel, Qt::AlignCenter);
    signalQualityLayout->addWidget(snrLabel);
    signalQualityLayout->setSpacing(10);

    updateSnrLevel(0);
    updateSyncStatus(uint8_t(DabSyncLevel::NoSync));

    QToolButton * setupButton = new QToolButton(this);
    setupAct = new QAction("Setup...", this);
    //setupAct->setStatusTip("Application settings");       // this is shown in status bar
    connect(setupAct, &QAction::triggered, setupDialog, &SetupDialog::show);
    clearServiceListAct = new QAction("Clear service list", this);

    bandScanAct = new QAction("Band scan...", this);
    //scanAct->setStatusTip("Seach for available service"); // this is shown in status bar
    connect(bandScanAct, &QAction::triggered, this, &MainWindow::bandScan);

    switchModeAct = new QAction("Expert mode", this);
    connect(switchModeAct, &QAction::triggered, this, &MainWindow::switchMode);

    menu = new QMenu(this);
    menu->addAction(setupAct);
    menu->addAction(bandScanAct);
    menu->addAction(switchModeAct);
    menu->addAction(clearServiceListAct);

    setupButton->setMenu(menu);
    //setupButton->setArrowType(Qt::DownArrow);
    setupButton->setText(QString(QChar(0x22EF)));    // MIDLINE horizontal ellipsis
    setupButton->setPopupMode(QToolButton::InstantPopup);

    QGridLayout * layout = new QGridLayout(widget);
    layout->addWidget(timeLabel, 0,0, Qt::AlignVCenter | Qt::AlignLeft);
    layout->addLayout(signalQualityLayout, 0,1,Qt::AlignVCenter | Qt::AlignRight);
    layout->addWidget(setupButton, 0,2,Qt::AlignVCenter | Qt::AlignRight);
    //layout->addWidget(new QLabel(QChar(0x2699)), 0,3,Qt::AlignVCenter | Qt::AlignRight);
    layout->setColumnStretch(0, 100);
    layout->setSpacing(20);
    ui->statusbar->addWidget(widget,1);   

    // set fonts
    QFont f;
    f.setPointSize(qRound(1.5 * ui->programTypeLabel->fontInfo().pointSize()));
    ui->ensembleLabel->setFont(f);
    f.setBold(true);
    ui->serviceLabel->setFont(f);

    // service list
    serviceList = new ServiceList;

    slModel = new SLModel(serviceList, this);
    connect(serviceList, &ServiceList::serviceAdded, slModel, &SLModel::addService);
    connect(clearServiceListAct, &QAction::triggered, serviceList, &ServiceList::clear);
    connect(serviceList, &ServiceList::empty, slModel, &SLModel::clear);
    //ui->serviceListView->setHeaderHidden(true);

    //ui->serviceListView->setModel(serviceListModel);
    ui->serviceListView->setModel(slModel);
    ui->serviceListView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->serviceListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(ui->serviceListView->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::serviceListCurrentChanged);

    // fill channel list
    dabChannelList_t::const_iterator i = DabTables::channelList.constBegin();
    while (i != DabTables::channelList.constEnd()) {
        ui->channelCombo->addItem(i.value(), i.key());
        ++i;
    }
    ui->channelCombo->setCurrentIndex(-1);
    ui->channelCombo->setDisabled(true);

    clearEnsembleInformationLabels();
    clearServiceInformationLabels();
    ui->dynamicLabel->setText("");

    ui->audioEncodingLabel->setToolTip("Audio coding");

#if USE_SLS_FRAME
    ui->slsView->setMinimumSize(QSize(322, 242));
#else
    ui->slsView->setMinimumSize(QSize(320, 240));
    ui->slsView->setFrameStyle(QFrame::NoFrame);
#endif    

    QPixmap pic;
    if (pic.load(":/resources/sls_logo.png"))
    {
        QGraphicsScene * scene = ui->slsView->scene();
        if (nullptr == scene)
        {
            scene = new QGraphicsScene(this);
            slsPixmapItem = scene->addPixmap(pic);

            ui->slsView->setScene(scene);
        }
        else
        {
            slsPixmapItem->setPixmap(pic);
        }
        scene->setSceneRect(pic.rect());
    }
    else
    {
        qDebug() << "Unable to load :/resources/sls_logo.png";
    }

    if (pic.load(":/resources/broadcast.png"))
    {
        ui->switchSourceLabel->setPixmap(pic);
    }
    else
    {
        qDebug() << "Unable to load :/resources/broadcast.png";
    }
    ui->switchSourceLabel->setToolTip("Click to change service source (ensemble)");
    ui->switchSourceLabel->setHidden(true);

    resize(minimumSizeHint());

    // threads
    radioControl = new RadioControl();
    radioControlThr = new QThread(this);
    radioControlThr->setObjectName("radioControlThr");
    radioControl->moveToThread(radioControlThr);
    connect(radioControlThr, &QThread::finished, radioControl, &QObject::deleteLater);
    radioControlThr->start();

    // initialize radio control
    if (!radioControl->init())
    {        
        qDebug() << "RadioControl() init failed";
        close();
        qApp->quit();
    }

    audioDecoder = new AudioDecoder(&audioBuffer, nullptr);
    audioDecoderThr = new QThread(this);
    audioDecoderThr->setObjectName("audioDecoderThr");
    audioDecoder->moveToThread(audioDecoderThr);
    connect(audioDecoderThr, &QThread::finished, audioDecoder, &QObject::deleteLater);
    audioDecoderThr->start();

    audioOutput = new AudioOutput(&audioBuffer);
#if (!defined AUDIOOUTPUT_USE_PORTAUDIO)
    audioOutThr = new QThread(this);    
    audioOutThr->setObjectName("audioOutThr");
    audioOutput->moveToThread(audioOutThr);
    connect(audioOutThr, &QThread::finished, audioOutput, &QObject::deleteLater);
    audioOutThr->start();
    audioOutThr->setPriority(QThread::HighestPriority);
#endif

    // Connect signals
    connect(ui->favoriteLabel, &FavoriteLabel::toggled, this, &MainWindow::favoriteToggled);
    connect(ui->switchSourceLabel, &ClickableLabel::clicked, this, &MainWindow::switchServiceSource);

    connect(radioControl, &RadioControl::ensembleInformation, this, &MainWindow::updateEnsembleInfo, Qt::QueuedConnection);
    connect(radioControl, &RadioControl::syncStatus, this, &MainWindow::updateSyncStatus, Qt::QueuedConnection);
    connect(radioControl, &RadioControl::snrLevel, this, &MainWindow::updateSnrLevel, Qt::QueuedConnection);
    connect(radioControl, &RadioControl::dabTime, this, &MainWindow::updateDabTime, Qt::QueuedConnection);
    connect(radioControl, &RadioControl::serviceListEntry, this, &MainWindow::updateServiceList, Qt::BlockingQueuedConnection);


    connect(radioControl, &RadioControl::dlDataGroup, dlDecoder, &DLDecoder::newDataGroup, Qt::QueuedConnection);
    connect(radioControl, &RadioControl::mscDataGroup, motDecoder, &MOTDecoder::newDataGroup, Qt::QueuedConnection);
    connect(radioControl, &RadioControl::newAudioService, this, &MainWindow::audioServiceChanged, Qt::QueuedConnection);
    connect(radioControl, &RadioControl::serviceChanged, dlDecoder, &DLDecoder::reset, Qt::QueuedConnection);
    connect(radioControl, &RadioControl::serviceChanged, motDecoder, &MOTDecoder::reset, Qt::QueuedConnection);
    connect(radioControl, &RadioControl::audioData, audioDecoder, &AudioDecoder::inputData, Qt::QueuedConnection);
    connect(dlDecoder, &DLDecoder::dlComplete, this, &MainWindow::updateDL, Qt::QueuedConnection);
    connect(motDecoder, &MOTDecoder::motObjectComplete, this, &MainWindow::updateSLS, Qt::QueuedConnection);   

    connect(audioDecoder, &AudioDecoder::audioParametersInfo, this, &MainWindow::updateAudioInfo, Qt::QueuedConnection);
    connect(radioControl, &RadioControl::newAudioService, audioDecoder, &AudioDecoder::start, Qt::QueuedConnection);

    // audio output is controlled by signals from decoder
    connect(this, &MainWindow::serviceRequest, audioDecoder, &AudioDecoder::stop, Qt::QueuedConnection);
    connect(audioDecoder, &AudioDecoder::startAudio, audioOutput, &AudioOutput::start, Qt::QueuedConnection);
    connect(audioDecoder, &AudioDecoder::stopAudio, audioOutput, &AudioOutput::stop, Qt::QueuedConnection);

    // tune procedure:
    // 1. mainwindow tune -> radiocontrol tune (this stops DAB SDR - tune to 0)
    // 2. radiocontrol tuneInputDevice -> inputdevice tune (reset of inut bufer and tune FE)
    // 3. inputDevice tuned -> radiocontrol start (this starts DAB SDR)
    // 4. notification to HMI
    connect(this, &MainWindow::serviceRequest, radioControl, &RadioControl::tuneService, Qt::QueuedConnection);

    // these two signals have to be connected in initInputDevice() - left here as comment
    // connect(radioControl, &RadioControl::tuneInputDevice, inputDevice, &InputDevice::tune, Qt::QueuedConnection);
    // connect(inputDevice, &InputDevice::tuned, radioControl, &RadioControl::start, Qt::QueuedConnection);
    connect(radioControl, &RadioControl::tuneDone, this, &MainWindow::tuneFinished, Qt::QueuedConnection);

    // input device connections
    initInputDevice(InputDeviceId::UNDEFINED);

    loadSettings();
}

MainWindow::~MainWindow()
{
    qDebug() << Q_FUNC_INFO;

    delete inputDevice;

    radioControlThr->quit();  // this deletes radioControl
    radioControlThr->wait();
    delete radioControlThr;

    audioDecoderThr->quit();  // this deletes audiodecoder
    audioDecoderThr->wait();
    delete audioDecoderThr;

#if (defined AUDIOOUTPUT_USE_RTAUDIO) || (defined AUDIOOUTPUT_USE_PORTAUDIO)
    delete audioOutput;
#else
    audioOutThr->quit();  // this deletes audiodecoder
    audioOutThr->wait();
    delete audioOutThr;
#endif

    delete motDecoder;
    delete dlDecoder;

    delete serviceList;

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();

    qDebug() << Q_FUNC_INFO << "stopping all processing";
    emit serviceRequest(0,0,0);
    event->accept();
}


void MainWindow::showEvent(QShowEvent *event)
{
    QGraphicsScene * scene = ui->slsView->scene();
    if (nullptr != scene)
    {
        ui->slsView->fitInViewTight(scene->itemsBoundingRect(), Qt::KeepAspectRatio);
    }

    QMainWindow::showEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    if (ui->dynamicLabel->width() < ui->dynamicLabel->fontMetrics().boundingRect(ui->dynamicLabel->text()).width())
    {
        ui->dynamicLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }
    else
    {
        ui->dynamicLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    }

    QGraphicsScene * scene = ui->slsView->scene();
    if (nullptr != scene)
    {
        ui->slsView->fitInViewTight(scene->itemsBoundingRect(), Qt::KeepAspectRatio);
    }

    QMainWindow::resizeEvent(event);
}

void MainWindow::inputDeviceReady()
{
    ui->channelCombo->setEnabled(true);    
}

void MainWindow::updateEnsembleInfo(const RadioControlEnsemble &ens)
{
    ui->ensembleLabel->setText(ens.label);
    ui->ensembleLabel->setToolTip(QString("<b>Ensemble:</b> %1<br>"
                                          "<b>Short label:</b> %2<br>"
                                          "<b>ECC:</b> 0x%3<br>"
                                          "<b>EID:</b> 0x%4<br>"
                                          "<b>Country:</b> %5")
                                  .arg(ens.label)
                                  .arg(ens.labelShort)
                                  .arg(QString("%1").arg(ens.ecc, 2, 16, QChar('0')).toUpper())
                                  .arg(QString("%1").arg(ens.eid, 4, 16, QChar('0')).toUpper())
                                  .arg(DabTables::getCountryName(ens.ueid)));
}

void MainWindow::updateSyncStatus(uint8_t sync)
{
    if (DabSyncLevel::FullSync > DabSyncLevel(sync))
    {   // hide time when no sync
        timeLabel->setText("");
    }
    syncLabel->setText(syncLevelLabels[sync]);
    syncLabel->setToolTip(syncLevelTooltip[sync]);
}

void MainWindow::updateSnrLevel(float snr)
{
    // limit SNR to progressbar maximum
    int snr10 = qRound(snr*10);
    if (snr10 > snrProgress->maximum())
    {
        snr10 = snrProgress->maximum();
    }
    snrProgress->setValue(snr10);

    snrLabel->setText(QString("%1 dB").arg(snr, 0, 'f', 1));
#ifndef __APPLE__
    // progressbar styling -> it does not look good on Apple
    if (static_cast<int>(SNR10Threhold::SNR_BAD) > snr10)
    {   // bad SNR
        snrProgress->setStyleSheet(snrProgressStylesheet[0]);
    }
    else if (static_cast<int>(SNR10Threhold::SNR_GOOD) > snr10)
    {   // medium SNR
        snrProgress->setStyleSheet(snrProgressStylesheet[1]);
    }
    else
    {   // good SNR
        snrProgress->setStyleSheet(snrProgressStylesheet[2]);
    }
#endif
}

void MainWindow::updateServiceList(const RadioControlServiceListEntry & slEntry)
{
    if (slEntry.TMId != DabTMId::StreamAudio)
    {  // do nothing - data services not supported
        return;
    }

    // add to service list
    serviceList->addService(slEntry);
}

void MainWindow::updateDL(const QString & dl)
{
    ui->dynamicLabel->setText(dl);
    if (ui->dynamicLabel->width() < ui->dynamicLabel->fontMetrics().boundingRect(dl).width())
    {
        ui->dynamicLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }
    else
    {
        ui->dynamicLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    }
}

void MainWindow::updateSLS(const QByteArray & b)
{
    QPixmap pic;
    if (pic.loadFromData(b))
    {
        QGraphicsScene * scene = ui->slsView->scene();
        if (nullptr == scene)
        {
            //qDebug() << Q_FUNC_INFO << "New graphisc scene";
            scene = new QGraphicsScene(this);
            slsPixmapItem = scene->addPixmap(pic);

            ui->slsView->setScene(scene);
        }
        else
        {
            slsPixmapItem->setPixmap(pic);
        }

        scene->setSceneRect(pic.rect());
        ui->slsView->fitInViewTight(pic.rect(), Qt::KeepAspectRatio);
    }
}

void MainWindow::updateDabTime(const QDateTime & d)
{
    timeLabel->setText(d.toString(QString("dddd, dd.MM.yyyy, hh:mm")));
}

void MainWindow::updateAudioInfo(const struct AudioParameters & params)
{
    switch (params.coding)
    {
    case AudioCoding::MP2:
        ui->audioEncodingLabel->setText("MP2");
        ui->audioEncodingLabel->setToolTip(QString("<b>DAB audio encoding</b><br>%1").arg("MPEG-1 layer 2"));
        break;
    case AudioCoding::AACLC:
        ui->audioEncodingLabel->setText("AAC-LC");
        ui->audioEncodingLabel->setToolTip(QString("<b>DAB+ audio encoding</b><br>%1").arg("MPEG-4 Low Complexity AAC"));
        break;
    case AudioCoding::HEAAC:
        ui->audioEncodingLabel->setText("HE-AAC");
        ui->audioEncodingLabel->setToolTip(QString("<b>DAB+ audio encoding</b><br>%1").arg("MPEG-4 High Efficiency AAC"));
        break;
    case AudioCoding::HEAACv2:
        ui->audioEncodingLabel->setText("HE-AACv2");
        ui->audioEncodingLabel->setToolTip(QString("<b>DAB+ audio encoding</b><br>%1").arg("MPEG-4 High Efficiency AAC v2"));
        break;
    }

    if (params.stereo)
    {
        ui->stereoLabel->setText("STEREO");
        if (AudioCoding::MP2 == params.coding)
        {
            ui->stereoLabel->setToolTip(QString("<b>Audio signal</b><br>%1Stereo<br>Sample rate: %2 kHz")
                    .arg(params.sbr ? "Joint " : "")
                    .arg(params.sampleRateKHz)
                );
        }
        else
        {
            ui->stereoLabel->setToolTip(QString("<b>Audio signal</b><br>Stereo (PS %1)<br>Sample rate: %2 kHz (SBR %3)")
                    .arg(params.parametricStereo? "on" : "off")
                    .arg(params.sampleRateKHz)
                    .arg(params.sbr ? "on" : "off")
                );
        }
    }
    else
    {
        ui->stereoLabel->setText("MONO");
        if (AudioCoding::MP2 == params.coding)
        {
            ui->stereoLabel->setToolTip(QString("<b>Audio signal</b><br>Mono<br>Sample rate: %1 kHz")
                        .arg(params.sampleRateKHz)
                    );
        }
        else
        {
            ui->stereoLabel->setToolTip(QString("<b>Audio signal</b><br>Mono<br>Sample rate: %1 kHz (SBR: %2)")
                        .arg(params.sampleRateKHz)
                        .arg(params.sbr ? "on" : "off")
                    );
        }
    }
}

void MainWindow::onChannelSelection()
{
    SId.value = 0;
    clearEnsembleInformationLabels();
    ui->frequencyLabel->setText("Tuning...  ");

    updateSyncStatus(uint8_t(DabSyncLevel::NoSync));
    updateSnrLevel(0);

    onServiceSelection();
}

void MainWindow::onServiceSelection()
{
    clearServiceInformationLabels();
    dlDecoder->reset();
    ui->dynamicLabel->setText("");    
    ui->switchSourceLabel->setHidden(true);
    motDecoder->reset();    
    QPixmap pic;
    if (pic.load(":/resources/sls_logo.png"))
    {
        QGraphicsScene * scene = ui->slsView->scene();
        if (nullptr == scene)
        {
            //qDebug() << Q_FUNC_INFO << "New graphisc scene";
            scene = new QGraphicsScene(this);
            slsPixmapItem = scene->addPixmap(pic);

            ui->slsView->setScene(scene);
        }
        else
        {
            slsPixmapItem->setPixmap(pic);
        }
        scene->setSceneRect(pic.rect());
        ui->slsView->fitInViewTight(pic.rect(), Qt::KeepAspectRatio);
    }
    else
    {
        qDebug() << "Unable to load :/resources/sls_logo.png";
    }
}

void MainWindow::on_channelCombo_currentIndexChanged(int index)
{
    if (frequency != ui->channelCombo->itemData(index).toUInt())
    {
        // reset UI
        onChannelSelection();

        emit serviceRequest(ui->channelCombo->itemData(index).toUInt(), 0, 0);
    }
}

void MainWindow::tuneFinished(uint32_t freq)
{  // this slot is called when tune is complete
    qDebug() << Q_FUNC_INFO << freq;

    frequency = freq;
    if (freq != 0)
    {
        ui->frequencyLabel->setText(QString("%1 MHz").arg(freq/1000.0, 3, 'f', 3, QChar('0')));
        isPlaying = true;
        setupDialog->enableFileSelection(false);
    }
    else
    {   // this can only happen when device is changed
        ui->frequencyLabel->setText("");
        isPlaying = false;
        setupDialog->enableFileSelection(true);
        if (deviceChangeRequested)
        {
            initInputDevice(inputDeviceId);
        }
    }
}

void MainWindow::onEndOfFile()
{
    timeLabel->setText("End of file");
    // tune to 0
    if (!fileLooping)
    {
        ui->channelCombo->setCurrentIndex(-1);
        setupDialog->enableFileSelection(true);
    }
}

void MainWindow::onRawFileStop()
{
    timeLabel->setText("End of file");
    // tune to 0
    ui->channelCombo->setCurrentIndex(-1);
}


void MainWindow::serviceListCurrentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    //if (current == previous) return;
    //Q_UNUSED(previous);
    //qDebug() << Q_FUNC_INFO << current << previous;
    const SLModel * model = reinterpret_cast<const SLModel*>(current.model());

    //qDebug() << current << model->getId(current) << previous << model->getId(previous);
    if (model->getId(current) == model->getId(previous))
    {
        return;
    }

    if (model->isService(current))
    {
        //qDebug() << Q_FUNC_INFO << "isService ID =" << model->getId(current);
        ServiceListConstIterator it = serviceList->findService(model->getId(current));
        if (serviceList->serviceListEnd() != it)
        {
            SId = (*it)->SId();
            SCIdS = (*it)->SCIdS();
            uint32_t newFrequency = (*it)->getEnsemble()->frequency();

            if (newFrequency != frequency)
            {
               frequency = newFrequency;

                // change combo
                int idx = 0;
                dabChannelList_t::const_iterator it = DabTables::channelList.constBegin();
                while (it != DabTables::channelList.constEnd()) {
                    if (it.key() == frequency)
                    {
                        ui->channelLabel->setText(it.value());
                        break;
                    }
                    ++it;
                    ++idx;
                }
                ui->channelCombo->setCurrentIndex(idx);


                // reset UI
                clearEnsembleInformationLabels();
                ui->frequencyLabel->setText("Tuning...  ");
                updateSyncStatus(uint8_t(DabSyncLevel::NoSync));
                updateSnrLevel(0);
            }                                           
            onServiceSelection();
            emit serviceRequest(frequency, SId.value, SCIdS);
        }
    }
#if 0
    QVariant data = current.model()->data(current, Qt::UserRole);
    RadioControlServiceListEntry * servicePtr = reinterpret_cast<RadioControlServiceListEntry *>(data.value<void*>());
    SId = servicePtr->SId;
    SCIdS = servicePtr->SCIdS;

    onServiceSelection();
    emit serviceRequest(frequency, SId.value, SCIdS);
#endif
}

void MainWindow::audioServiceChanged(const RadioControlAudioService &s)
{
    if (s.SId.value == SId.value)
    {
        if (s.label.isEmpty())
        {   // service component not valid -> shoudl not happen
            return;
        }
        // set service name in UI until information arrives from decoder

        uint64_t id = ServiceListItem::getId(s);
        ServiceListConstIterator it = serviceList->findService(id);
        if (it != serviceList->serviceListEnd())
        {
            ui->favoriteLabel->setActive((*it)->isFavorite());
            int numEns = (*it)->numEnsembles();
            if (numEns > 1)
            {
                ui->switchSourceLabel->setVisible(true);
                int current = (*it)->currentEnsembleIdx();
                const EnsembleListItem * ens = (*it)->getEnsemble(current+1);  // get next ensemble
                ui->switchSourceLabel->setToolTip(QString("<b>Ensemble %1/%2</b><br>Click for switching to:<br><i>%3</i>")
                                                  .arg(current+1)
                                                  .arg(numEns)
                                                  .arg(ens->label()));
            }
        }
        ui->serviceLabel->setText(s.label);
        ui->serviceLabel->setToolTip(QString("<b>Service:</b> %1<br>"
                                             "<b>Short label:</b> %2<br>"
                                             "<b>SId:</b> 0x%3<br>"
                                             "<b>SCIdS:</b> %4<br>"
                                             "<b>Language:</b> %5<br>"
                                             "<b>Country:</b> %6")
                                     .arg(s.label)
                                     .arg(s.labelShort)
                                     .arg(QString("%1").arg(s.SId.prog.countryServiceRef, 4, 16, QChar('0')).toUpper() )
                                     .arg(s.SCIdS)
                                     .arg(DabTables::getLangName(s.lang))
                                     .arg(DabTables::getCountryName(s.SId.value)));        

        if ((s.pty.d != 0) && (s.pty.d != s.pty.s))
        {   // dynamic PTy available != static PTy
            ui->programTypeLabel->setText(DabTables::getPtyName(s.pty.d));
            ui->programTypeLabel->setToolTip(QString("<b>Programme Type</b><br>"
                                                     "%1 (dynamic)<br>"
                                                     "%2 (static)")
                                             .arg(DabTables::getPtyName(s.pty.d))
                                             .arg(DabTables::getPtyName(s.pty.s)));

        }
        else
        {
            ui->programTypeLabel->setText(DabTables::getPtyName(s.pty.s));
            ui->programTypeLabel->setToolTip(QString("<b>Programme Type</b><br>"
                                                     "%1")
                                             .arg(DabTables::getPtyName(s.pty.s)));

        }

        QString label;
        QString toolTip;
        if (s.protection.isEEP())
        {  // EEP
            if (s.protection.level < DabProtectionLevel::EEP_1B)
            {  // EEP x-A
                label = QString("EEP %1-%2").arg(int(s.protection.level) - int(DabProtectionLevel::EEP_1A) + 1).arg("A");

            }
            else
            {  // EEP x+B
                label = QString("EEP %1-%2").arg(int(s.protection.level) - int(DabProtectionLevel::EEP_1B) + 1).arg("B");
            }
            toolTip = QString("<B>Error protection</b><br>"
                              "%1<br>Coderate: %2/%3<br>"
                              "Capacity units: %4 CU")
                                            .arg(label)
                                            .arg(s.protection.codeRateUpper)
                                            .arg(s.protection.codeRateLower)
                                            .arg(s.SubChSize);
        }
        else
        {  // UEP
            label = QString("UEP #%1").arg(s.protection.uepIndex);
            toolTip = QString("<B>Error protection</b><br>"
                              "%1<br>Protection level: %2<br>"
                              "Capacity units: %3 CU")
                                            .arg(label)
                                            .arg(int(s.protection.level))
                                            .arg(s.SubChSize);
        }
        ui->protectionLabel->setText(label);
        ui->protectionLabel->setToolTip(toolTip);

        QString br = QString("%1 kbps").arg(s.bitRate);
        ui->audioBitrateLabel->setText(br);
        ui->audioBitrateLabel->setToolTip(QString("<b>Service bitrate</b><br>Audio & data: %1").arg(br));

        //ui->serviceListView->selectionModel()->setCurrentIndex(item->index(), QItemSelectionModel::Select | QItemSelectionModel::Current);
        ui->serviceListView->setFocus();
    }
    else
    {   // sid it not equal to selected sid -> should not happen
        ui->serviceListView->clearSelection();
        return;
    }
}

void MainWindow::clearEnsembleInformationLabels()
{
    timeLabel->setText("");    
    ui->ensembleLabel->setText("No ensemble");
    ui->ensembleLabel->setToolTip("No ensemble tuned");
    ui->ensembleInfoLabel->setText("");
    ui->frequencyLabel->setText("");
}

void MainWindow::clearServiceInformationLabels()
{    
    ui->serviceLabel->setText("No service");
    ui->favoriteLabel->setActive(false);
    ui->serviceLabel->setToolTip("No service playing");
    ui->programTypeLabel->setText("");
    ui->audioEncodingLabel->setText("");
    ui->audioEncodingLabel->setToolTip("");
    ui->stereoLabel->setText("");
    ui->stereoLabel->setToolTip("");
    ui->protectionLabel->setText("");
    ui->protectionLabel->setToolTip("");
    ui->audioBitrateLabel->setText("");
    ui->audioBitrateLabel->setToolTip("");

}

void MainWindow::changeInputDevice(const InputDeviceId & d)
{
    qDebug() << Q_FUNC_INFO << int(d);
    if (d != inputDeviceId)
    {
        deviceChangeRequested = true;
        inputDeviceId = d;
        if (isPlaying)
        { // stop
            // tune to 0
            ui->channelCombo->setCurrentIndex(-1);
            ui->channelCombo->setDisabled(true);  // enabled when device is ready
            ui->channelLabel->setText("");
        }
        else
        { // device is not playing
            initInputDevice(d);
        }
    }
}

void MainWindow::initInputDevice(const InputDeviceId & d)
{
    qDebug() << Q_FUNC_INFO << int(d);

    deviceChangeRequested = false;
    if (nullptr != inputDevice)
    {
        delete inputDevice;
        setupDialog->resetFilename();
    }
    switch (d)
    {
    case InputDeviceId::UNDEFINED:
        // do nothing
        inputDevice = nullptr;
        ui->channelCombo->setDisabled(true);   // it will be enabled when device is ready
        ui->channelLabel->setText("");
        //setupDialog->setInputDevice(inputDeviceId); // this emits device change
        break;
    case InputDeviceId::RTLSDR:
    {
        inputDevice = new RtlSdrInput();
        if (inputDevice->isAvailable())
        {  // rtl sdr is available
            inputDeviceId = InputDeviceId::RTLSDR;
            setupDialog->enableRtlSdrInput(true);
            setupDialog->setInputDevice(inputDeviceId); // this emits device change

            // tuning procedure
            connect(radioControl, &RadioControl::tuneInputDevice, inputDevice, &InputDevice::tune, Qt::QueuedConnection);
            connect(inputDevice, &InputDevice::tuned, radioControl, &RadioControl::start, Qt::QueuedConnection);

            // HMI
            connect(inputDevice, &InputDevice::deviceReady, this, &MainWindow::inputDeviceReady, Qt::QueuedConnection);

            // setup dialog
            connect(dynamic_cast<RtlSdrInput*>(inputDevice), &RtlSdrInput::gainListAvailable, setupDialog, &SetupDialog::setGainValues);
            connect(setupDialog, &SetupDialog::setAutoGain, dynamic_cast<RtlSdrInput*>(inputDevice), &RtlSdrInput::setGainAutoMode);
            connect(setupDialog, &SetupDialog::setGain, dynamic_cast<RtlSdrInput*>(inputDevice), &RtlSdrInput::setGain);

            static_cast<RtlSdrInput *>(inputDevice)->openDevice();
        }
        else
        {
            delete inputDevice;
            inputDevice = nullptr;
            inputDeviceId = InputDeviceId::UNDEFINED;
            setupDialog->enableRtlSdrInput(false);
            setupDialog->setInputDevice(inputDeviceId); // this emits device change
        }
    }
        break;
    case InputDeviceId::RAWFILE:
    {
        inputDevice = new RawFileInput();

        inputDeviceId = InputDeviceId::RAWFILE;
        setupDialog->enableRtlSdrInput(true);
        setupDialog->setInputDevice(inputDeviceId); // this emits device change

        // tuning procedure
        connect(radioControl, &RadioControl::tuneInputDevice, inputDevice, &InputDevice::tune, Qt::QueuedConnection);
        connect(inputDevice, &InputDevice::tuned, radioControl, &RadioControl::start, Qt::QueuedConnection);

        // HMI
        connect(inputDevice, &InputDevice::deviceReady, this, &MainWindow::inputDeviceReady, Qt::QueuedConnection);
        connect(dynamic_cast<RawFileInput*>(inputDevice), &RawFileInput::endOfFile, this, &MainWindow::onEndOfFile);

        // setup dialog
        connect(setupDialog, &SetupDialog::rawFileSelected, dynamic_cast<RawFileInput*>(inputDevice), &RawFileInput::openDevice);
        connect(setupDialog, &SetupDialog::sampleFormat, dynamic_cast<RawFileInput*>(inputDevice), &RawFileInput::setFileFormat);
        setupDialog->enableFileSelection(true);
    }
        break;
    }
}

void MainWindow::loadSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, appName, appName);

    // load servicelist
    serviceList->load(settings);
    expertMode = settings.value("ExpertMode").toBool();
    setExpertMode(expertMode);

    int inDevice = settings.value("inputDeviceId", int(InputDeviceId::UNDEFINED)).toInt();
    if (InputDeviceId::UNDEFINED != static_cast<InputDeviceId>(inDevice))
    {        
        // if input device has switched to what was stored and it is RTLSDR
        setupDialog->setInputDevice(static_cast<InputDeviceId>(inDevice));
        if ((static_cast<InputDeviceId>(inDevice) == inputDeviceId) && (InputDeviceId::RTLSDR == inputDeviceId))
        {   // channel is only restored for RTL SDR at the moment
            SId.value = settings.value("SID", 0).toInt();
            SCIdS = settings.value("SCIdS", 0).toInt();            
            uint64_t id = ServiceListItem::getId(SId.value, SCIdS);

            // we need to find the item in model and select it
            const SLModel * model = reinterpret_cast<const SLModel*>(ui->serviceListView->model());
            QModelIndex index;
            for (int r = 0; r < model->rowCount(); ++r)
            {
                index = model->index(r, 0);
                if (model->getId(index) == id)
                {   // found
                    ui->serviceListView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select | QItemSelectionModel::Current);
                    ui->serviceListView->setFocus();
                    break;
                }
            }
        }
    }
}

void MainWindow::saveSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, appName, appName);
    settings.setValue("inputDeviceId", int(inputDeviceId));

    QModelIndex current = ui->serviceListView->currentIndex();
    const SLModel * model = reinterpret_cast<const SLModel*>(current.model());
    if (model->isService(current))
    {
        ServiceListConstIterator it = serviceList->findService(model->getId(current));
        if (serviceList->serviceListEnd() != it)
        {
            SId = (*it)->SId();
            SCIdS = (*it)->SCIdS();
            frequency = (*it)->getEnsemble()->frequency();

            settings.setValue("SID", SId.value);
            settings.setValue("SCIdS", SCIdS);
            settings.setValue("Frequency", frequency);
        }
    }
    settings.setValue("ExpertMode", expertMode);

    serviceList->save(settings);

    settings.sync();
}


void MainWindow::favoriteToggled(bool checked)
{
    QModelIndex current = ui->serviceListView->currentIndex();            
    const SLModel * model = reinterpret_cast<const SLModel*>(current.model());
    uint64_t id = model->getId(current);
    serviceList->setServiceFavorite(id, checked);

    slModel->sort(0);

    // find new position of current service and select it
    QModelIndex index;
    for (int r = 0; r < model->rowCount(); ++r)
    {
        index = model->index(r, 0);
        if (model->getId(index) == id)
        {   // found
            ui->serviceListView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select | QItemSelectionModel::Current);
            ui->serviceListView->setFocus();
            break;
        }
    }
}

void MainWindow::switchServiceSource()
{
    QModelIndex current = ui->serviceListView->currentIndex();
    const SLModel * model = reinterpret_cast<const SLModel*>(current.model());
    uint64_t id = model->getId(current);
    ServiceListConstIterator it = serviceList->findService(id);
    if (it != serviceList->serviceListEnd())
    {
        // swith to next ensemble &  get ensemble frequency
        uint32_t newFrequency = (*it)->switchEnsemble()->frequency();
        if (newFrequency)
        {
            if (newFrequency != frequency)
            {
               frequency = newFrequency;

                // change combo
                int idx = 0;
                dabChannelList_t::const_iterator it = DabTables::channelList.constBegin();
                while (it != DabTables::channelList.constEnd()) {
                    if (it.key() == frequency)
                    {
                        ui->channelLabel->setText(it.value());
                        break;
                    }
                    ++it;
                    ++idx;
                }
                ui->channelCombo->setCurrentIndex(idx);


                // reset UI
                clearEnsembleInformationLabels();
                ui->frequencyLabel->setText("Tuning...  ");
                updateSyncStatus(uint8_t(DabSyncLevel::NoSync));
                updateSnrLevel(0);
            }
            frequency = newFrequency;
            onServiceSelection();
            emit serviceRequest(frequency, SId.value, SCIdS);
        }
    }
}

void MainWindow::switchMode()
{   // toggle expert mode
    expertMode = !expertMode;
    setExpertMode(expertMode);
}

void MainWindow::setExpertMode(bool ena)
{
    if (ena)
    {
        switchModeAct->setText("Basic mode");
    }
    else
    {
        switchModeAct->setText("Expert mode");
    }
    ui->channelCombo->setVisible(ena);
    ui->channelLabel->setVisible(!ena);
}

void MainWindow::bandScan()
{
    //BandScanDialog * dialog = new BandScanDialog(this);
//    dialog->setModal(true);
//    dialog->show();
    BandScanDialog dialog(this);
    qDebug() << Q_FUNC_INFO << dialog.exec();
}
