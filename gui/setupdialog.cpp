#include <QFileDialog>
#include <QFile>
#include <QDebug>
#include <QStandardItemModel>

#include "setupdialog.h"
#include "./ui_setupdialog.h"

static const QString NO_FILE("No file selected");

SetupDialog::SetupDialog(QWidget *parent) : QDialog(parent), ui(new Ui::SetupDialog)
{
    ui->setupUi(this);

    // remove question mark from titlebar
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->tabWidget->setTabText(SetupDialogTabs::Device, "Input device");
    ui->tabWidget->setTabText(SetupDialogTabs::Announcement, "Announcements");

    ui->inputCombo->addItem("RTL SDR", QVariant(int(InputDeviceId::RTLSDR)));
#if HAVE_AIRSPY
    ui->inputCombo->addItem("Airspy", QVariant(int(InputDeviceId::AIRSPY)));
#endif
#if HAVE_SOAPYSDR
    ui->inputCombo->addItem("Soapy SDR", QVariant(int(InputDeviceId::SOAPYSDR)));
#endif
    ui->inputCombo->addItem("RTL TCP", QVariant(int(InputDeviceId::RTLTCP)));
#if HAVE_RARTTCP
    ui->inputCombo->addItem("RaRT TCP", QVariant(int(InputDeviceId::RARTTCP)));
#endif
    ui->inputCombo->addItem("Raw file", QVariant(int(InputDeviceId::RAWFILE)));
    ui->inputCombo->setCurrentIndex(-1);  // undefined

    ui->fileNameLabel->setText(NO_FILE);
    m_rawfilename = "";

    ui->fileFormatCombo->insertItem(int(RawFileInputFormat::SAMPLE_FORMAT_U8), "Unsigned 8 bits");
    ui->fileFormatCombo->insertItem(int(RawFileInputFormat::SAMPLE_FORMAT_S16), "Signed 16 bits");

    // this has to be aligned with mainwindow
    ui->loopCheckbox->setChecked(false);

    ui->statusLabel->setText("<span style=\"color:red\">No device connected</span>");            

    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    QRegularExpression ipRegex ("^" + ipRange
                               +                                + "(\\." + ipRange + ")"
                               +                                + "(\\." + ipRange + ")"
                               +                                + "(\\." + ipRange + ")$");
    QRegularExpressionValidator *ipValidator = new QRegularExpressionValidator(ipRegex, this);
    ui->rtltcpIpAddressEdit->setValidator(ipValidator);
    ui->rarttcpIpAddressEdit->setValidator(ipValidator);

    // set announcement combos
    //    QGridLayout *mainLayout = new QGridLayout;
    //    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    //    mainLayout->addWidget(ui->tabWidget, 0, 0);
    //    setLayout(mainLayout);
#if  0
    int row = 0;
    int column = 0;
    QGridLayout *announcementLayout = new QGridLayout;
    for (int ann = 0; ann < static_cast<int>(DabAnnouncement::Undefined); ++ann)
    {
        //if (static_cast<int>(DabAnnouncement::Alarm) != ann)
        {
            m_announcementCheckBox[ann] = new QCheckBox();
            m_announcementCheckBox[ann]->setText(DabTables::getAnnouncementName(static_cast<DabAnnouncement>(ann)));
            m_announcementCheckBox[ann]->setChecked(true);

            if (static_cast<int>(DabAnnouncement::Alarm) == ann)
            {
                m_announcementCheckBox[ann]->setDisabled(true);
            }

            announcementLayout->addWidget(m_announcementCheckBox[ann], row++, column);
            if (row >= (static_cast<int>(DabAnnouncement::Undefined)+1)/2)
            {
                row = 0;
                column = 1;
            }
        }
    }
    ui->tabAnnouncement->setLayout(announcementLayout);
#endif

    QGridLayout *gridLayout = new QGridLayout;
    // alarm announcements
    int ann = static_cast<int>(DabAnnouncement::Alarm);
    m_announcementCheckBox[ann] = new QCheckBox();
    m_announcementCheckBox[ann]->setText(DabTables::getAnnouncementName(static_cast<DabAnnouncement>(ann)));
    m_announcementCheckBox[ann]->setChecked(true);
    m_announcementCheckBox[ann]->setDisabled(true);
    gridLayout->addWidget(m_announcementCheckBox[ann], 0, 0);

    ann = static_cast<int>(DabAnnouncement::AlarmTest);
    m_announcementCheckBox[ann] = new QCheckBox();
    m_announcementCheckBox[ann]->setText(DabTables::getAnnouncementName(static_cast<DabAnnouncement>(ann)));
    connect(m_announcementCheckBox[ann], &QCheckBox::clicked, this, &SetupDialog::onAnnouncementClicked);
    gridLayout->addWidget(m_announcementCheckBox[ann], 0, 1);

    QLabel * label = new QLabel("<br>Note: Alarm announcement cannot be disabled.");
    gridLayout->addWidget(label, 1, 0, 1, 2);
    QGroupBox * groupBox = new QGroupBox("Alarm Announcements");
    groupBox->setLayout(gridLayout);
    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(groupBox);

    // regular announcements
    int row = 0;
    int column = 0;
    gridLayout = new QGridLayout;
    for (int ann = static_cast<int>(DabAnnouncement::Alarm)+1; ann < static_cast<int>(DabAnnouncement::AlarmTest); ++ann)
    {
        m_announcementCheckBox[ann] = new QCheckBox();
        m_announcementCheckBox[ann]->setText(DabTables::getAnnouncementName(static_cast<DabAnnouncement>(ann)));
        connect(m_announcementCheckBox[ann], &QCheckBox::clicked, this, &SetupDialog::onAnnouncementClicked);
        gridLayout->addWidget(m_announcementCheckBox[ann], row++, column);
        if (row > 4)
        {
            row = 0;
            column = 1;
        }
    }
    groupBox = new QGroupBox("Regular Announcements");
    groupBox->setLayout(gridLayout);
    vLayout->addWidget(groupBox);

    QSpacerItem * verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
    vLayout->addItem(verticalSpacer);
    ui->tabAnnouncement->setLayout(vLayout);

    connect(ui->inputCombo, &QComboBox::currentIndexChanged, this, &SetupDialog::onInputChanged);
    connect(ui->openFileButton, &QPushButton::clicked, this, &SetupDialog::onOpenFileButtonClicked);

    connect(ui->connectButton, &QPushButton::clicked, this, &SetupDialog::onConnectDeviceClicked);

    connect(ui->rtlsdrGainSlider, &QSlider::valueChanged, this, &SetupDialog::onRtlSdrGainSliderChanged);
    connect(ui->rtlsdrGainModeHw, &QRadioButton::toggled, this, &SetupDialog::onRtlGainModeToggled);
    connect(ui->rtlsdrGainModeSw, &QRadioButton::toggled, this, &SetupDialog::onRtlGainModeToggled);
    connect(ui->rtlsdrGainModeManual, &QRadioButton::toggled, this, &SetupDialog::onRtlGainModeToggled);

    connect(ui->rtltcpGainSlider, &QSlider::valueChanged, this, &SetupDialog::onRtlTcpGainSliderChanged);
    connect(ui->rtltcpGainModeHw, &QRadioButton::toggled, this, &SetupDialog::onTcpGainModeToggled);
    connect(ui->rtltcpGainModeSw, &QRadioButton::toggled, this, &SetupDialog::onTcpGainModeToggled);
    connect(ui->rtltcpGainModeManual, &QRadioButton::toggled, this, &SetupDialog::onTcpGainModeToggled);
    connect(ui->rtltcpIpAddressEdit, &QLineEdit::editingFinished, this, &SetupDialog::onRtlTcpIpAddrEditFinished);
    connect(ui->rtltcpIpPortSpinBox, &QSpinBox::valueChanged, this, &SetupDialog::onRtlTcpPortValueChanged);

    connect(ui->loopCheckbox, &QCheckBox::stateChanged, [=](int val) { m_settings.rawfile.loopEna = (Qt::Unchecked != val); });
    connect(ui->fileFormatCombo, &QComboBox::currentIndexChanged, this, &SetupDialog::onRawFileFormatChanged);

#if HAVE_AIRSPY
    connect(ui->airspySensitivityGainSlider, &QSlider::valueChanged, this, &SetupDialog::onAirspySensitivityGainSliderChanged);
    connect(ui->airspyIFGainSlider, &QSlider::valueChanged, this, &SetupDialog::onAirspyIFGainSliderChanged);
    connect(ui->airspyLNAGainSlider, &QSlider::valueChanged, this, &SetupDialog::onAirspyLNAGainSliderChanged);
    connect(ui->airspyMixerGainSlider, &QSlider::valueChanged, this, &SetupDialog::onAirspyMixerGainSliderChanged);
    connect(ui->airspyLNAAGCCheckbox, &QCheckBox::stateChanged, this, &SetupDialog::onAirspyLNAAGCstateChanged);
    connect(ui->airspyMixerAGCCheckbox, &QCheckBox::stateChanged, this, &SetupDialog::onAirspyMixerAGCstateChanged);
    connect(ui->airspyGainModeHybrid, &QRadioButton::toggled, this, &SetupDialog::onAirspyModeToggled);
    connect(ui->airspyGainModeSw, &QRadioButton::toggled, this, &SetupDialog::onAirspyModeToggled);
    connect(ui->airspyGainModeManual, &QRadioButton::toggled, this, &SetupDialog::onAirspyModeToggled);
    connect(ui->airspyGainModeSensitivity, &QRadioButton::toggled, this, &SetupDialog::onAirspyModeToggled);
#endif

#if HAVE_SOAPYSDR
    connect(ui->soapysdrGainSlider, &QSlider::valueChanged, this, &SetupDialog::onSoapySdrGainSliderChanged);
    connect(ui->soapysdrDevArgs, &QLineEdit::editingFinished, this, &SetupDialog::onSoapySdrDevArgsEditFinished);
    connect(ui->soapySdrAntenna, &QLineEdit::editingFinished, this, &SetupDialog::onSoapySdrAntennaEditFinished);
    connect(ui->soapysdrChannelNum, &QSpinBox::editingFinished, this, &SetupDialog::onSoapySdrChannelEditFinished);
    connect(ui->soapysdrGainModeHw, &QRadioButton::toggled, this, &SetupDialog::onSoapySdrGainModeToggled);
    connect(ui->soapysdrGainModeSw, &QRadioButton::toggled, this, &SetupDialog::onSoapySdrGainModeToggled);
    connect(ui->soapysdrGainModeManual, &QRadioButton::toggled, this, &SetupDialog::onSoapySdrGainModeToggled);
#endif
    adjustSize();

    this->layout()->setSizeConstraint(QLayout::SetFixedSize);
}

SetupDialog::Settings SetupDialog::settings() const
{
    return m_settings;
}

void SetupDialog::setGainValues(const QList<float> &gainList)
{

    switch (m_settings.inputDevice)
    {
    case InputDeviceId::RTLSDR:
        m_rtlsdrGainList.clear();
        m_rtlsdrGainList = gainList;
        ui->rtlsdrGainSlider->setMinimum(0);
        ui->rtlsdrGainSlider->setMaximum(m_rtlsdrGainList.size()-1);
        ui->rtlsdrGainSlider->setValue((m_settings.rtlsdr.gainIdx >= 0) ? m_settings.rtlsdr.gainIdx : 0);
        break;
    case InputDeviceId::RTLTCP:
        m_rtltcpGainList.clear();
        m_rtltcpGainList = gainList;
        ui->rtltcpGainSlider->setMinimum(0);
        ui->rtltcpGainSlider->setMaximum(m_rtltcpGainList.size()-1);
        ui->rtltcpGainSlider->setValue((m_settings.rtltcp.gainIdx >= 0) ? m_settings.rtltcp.gainIdx : 0);
        break;
    case InputDeviceId::SOAPYSDR:
#if HAVE_SOAPYSDR
        m_soapysdrGainList.clear();
        m_soapysdrGainList = gainList;
        ui->soapysdrGainSlider->setMinimum(0);
        ui->soapysdrGainSlider->setMaximum(m_soapysdrGainList.size()-1);
        ui->soapysdrGainSlider->setValue((m_settings.soapysdr.gainIdx >= 0) ? m_settings.soapysdr.gainIdx : 0);
#endif // HAVE_SOAPYSDR
        break;
    case InputDeviceId::UNDEFINED:
    case InputDeviceId::RARTTCP:
    case InputDeviceId::RAWFILE:
    case InputDeviceId::AIRSPY:
        return;
    }
}

void SetupDialog::setSettings(const Settings &settings)
{
    m_settings = settings;
    setStatusLabel();
    emit newAnnouncementSettings(m_settings.announcementEna);
}

void SetupDialog::showEvent(QShowEvent *event)
{
    int index = ui->inputCombo->findData(QVariant(static_cast<int>(m_settings.inputDevice)));
    if (index < 0)
    {  // not found -> show rtlsdr as default
        index = ui->inputCombo->findData(QVariant(static_cast<int>(InputDeviceId::RTLSDR)));
    }
    ui->inputCombo->setCurrentIndex(index);

    if (!m_rtlsdrGainList.isEmpty())
    {
        ui->rtlsdrGainSlider->setValue(m_settings.rtlsdr.gainIdx);
        onRtlSdrGainSliderChanged(m_settings.rtlsdr.gainIdx);
    }
    else
    {
        ui->rtlsdrGainSlider->setValue(0);
        ui->rtlsdrGainValueLabel->setText("N/A");
    }
    if (!m_rtltcpGainList.isEmpty())
    {
        ui->rtltcpGainSlider->setValue(m_settings.rtltcp.gainIdx);
        onRtlTcpGainSliderChanged(m_settings.rtltcp.gainIdx);
    }
    else
    {
        ui->rtltcpGainSlider->setValue(0);
        ui->rtltcpGainValueLabel->setText("N/A");
    }

    switch (m_settings.rtlsdr.gainMode) {
    case RtlGainMode::Software:
        ui->rtlsdrGainModeSw->setChecked(true);
        break;
    case RtlGainMode::Hardware:
        ui->rtlsdrGainModeHw->setChecked(true);
        break;
    case RtlGainMode::Manual:
        ui->rtlsdrGainModeManual->setChecked(true);
        break;
    default:
        break;
    }

    switch (m_settings.rtltcp.gainMode) {
    case RtlGainMode::Software:
        ui->rtltcpGainModeSw->setChecked(true);
        break;
    case RtlGainMode::Hardware:
        ui->rtltcpGainModeHw->setChecked(true);
        break;
    case RtlGainMode::Manual:
        ui->rtltcpGainModeManual->setChecked(true);
        break;
    default:
        break;
    }

#if HAVE_AIRSPY
    switch (m_settings.airspy.gain.mode) {
    case AirpyGainMode::Software:
        ui->airspyGainModeSw->setChecked(true);
        break;
    case AirpyGainMode::Hybrid:
        ui->airspyGainModeHybrid->setChecked(true);
        break;
    case AirpyGainMode::Manual:
        ui->airspyGainModeManual->setChecked(true);
        break;
    case AirpyGainMode::Sensitivity:
        ui->airspyGainModeSensitivity->setChecked(true);
        break;
    default:
        break;
    }

    ui->airspySensitivityGainSlider->setValue(m_settings.airspy.gain.sensitivityGainIdx);
    ui->airspyIFGainSlider->setValue(m_settings.airspy.gain.ifGainIdx);
    ui->airspyLNAGainSlider->setValue(m_settings.airspy.gain.lnaGainIdx);
    ui->airspyMixerGainSlider->setValue(m_settings.airspy.gain.mixerGainIdx);
    ui->airspyMixerAGCCheckbox->setChecked(m_settings.airspy.gain.mixerAgcEna);
    ui->airspyLNAAGCCheckbox->setChecked(m_settings.airspy.gain.lnaAgcEna);
#endif

#if HAVE_SOAPYSDR
    switch (m_settings.soapysdr.gainMode) {
    case SoapyGainMode::Software:
        ui->soapysdrGainModeSw->setChecked(true);
        break;
    case SoapyGainMode::Hardware:
        ui->soapysdrGainModeHw->setChecked(true);
        break;
    case SoapyGainMode::Manual:
        ui->soapysdrGainModeManual->setChecked(true);
        break;
    default:
        break;
    }

    if (!m_soapysdrGainList.isEmpty())
    {
        ui->soapysdrGainSlider->setValue(m_settings.soapysdr.gainIdx);
        onSoapySdrGainSliderChanged(m_settings.soapysdr.gainIdx);
    }
    else
    {
        ui->soapysdrGainSlider->setValue(0);
        ui->soapysdrGainValueLabel->setText("N/A");
    }

    ui->soapysdrDevArgs->setText(m_settings.soapysdr.devArgs);
    ui->soapysdrChannelNum->setValue(m_settings.soapysdr.channel);
    ui->soapySdrAntenna->setText(m_settings.soapysdr.antenna);
#endif

    if (m_settings.rawfile.file.isEmpty())
    {
        ui->fileNameLabel->setText(NO_FILE);
        m_rawfilename = "";
        ui->fileNameLabel->setToolTip("");
    }
    else
    {
        ui->fileNameLabel->setText(QFileInfo(m_settings.rawfile.file).fileName());
        m_rawfilename = m_settings.rawfile.file;
        ui->fileNameLabel->setToolTip(m_settings.rawfile.file);
    }
    ui->loopCheckbox->setChecked(m_settings.rawfile.loopEna);
    ui->fileFormatCombo->setCurrentIndex(static_cast<int>(m_settings.rawfile.format));
    ui->rtltcpIpAddressEdit->setText(m_settings.rtltcp.tcpAddress);
    ui->rtltcpIpPortSpinBox->setValue(m_settings.rtltcp.tcpPort);
#if HAVE_RARTTCP
    ui->rarttcpIpAddressEdit->setText(m_settings.rarttcp.tcpAddress);
    ui->rarttcpIpPortSpinBox->setValue(m_settings.rarttcp.tcpPort);
#endif

    setStatusLabel();

    // announcments
    uint16_t announcementEna = m_settings.announcementEna | (1 << static_cast<int>(DabAnnouncement::Alarm));  // enable alarm
    for (int a = 0; a < static_cast<int>(DabAnnouncement::Undefined); ++a)
    {
        m_announcementCheckBox[a]->setChecked(announcementEna & 0x1);
        announcementEna >>= 1;
    }
}


void SetupDialog::onConnectDeviceClicked()
{
    ui->connectButton->setHidden(true);
    m_settings.inputDevice = static_cast<InputDeviceId>(ui->inputCombo->itemData(ui->inputCombo->currentIndex()).toInt());
    setStatusLabel();
    switch (m_settings.inputDevice)
    {
    case InputDeviceId::RTLSDR:
        activateRtlSdrControls(true);
        break;
    case InputDeviceId::RTLTCP:
        m_settings.rtltcp.tcpAddress = ui->rtltcpIpAddressEdit->text();
        m_settings.rtltcp.tcpPort = ui->rtltcpIpPortSpinBox->value();
        activateRtlTcpControls(true);
        break;
    case InputDeviceId::RARTTCP:
        break;
    case InputDeviceId::UNDEFINED:
        break;
    case InputDeviceId::RAWFILE:
        m_settings.rawfile.file = m_rawfilename;
        m_settings.rawfile.loopEna = ui->loopCheckbox->isChecked();
        m_settings.rawfile.format = static_cast<RawFileInputFormat>(ui->fileFormatCombo->currentIndex());
        break;
    case InputDeviceId::AIRSPY:
#if HAVE_AIRSPY
        activateAirspyControls(true);
#endif
        break;
    case InputDeviceId::SOAPYSDR:
#if HAVE_SOAPYSDR
        m_settings.soapysdr.devArgs = ui->soapysdrDevArgs->text();
        m_settings.soapysdr.channel = ui->soapysdrChannelNum->text().toInt();
        m_settings.soapysdr.antenna = ui->soapySdrAntenna->text();
        activateSoapySdrControls(true);
#endif
        break;
    }
    emit inputDeviceChanged(m_settings.inputDevice);
}

void SetupDialog::onRtlSdrGainSliderChanged(int val)
{
    ui->rtlsdrGainValueLabel->setText(QString("%1 dB").arg(m_rtlsdrGainList.at(val)));
    m_settings.rtlsdr.gainIdx = val;
    emit newInputDeviceSettings();
}

void SetupDialog::onRtlTcpGainSliderChanged(int val)
{
    ui->rtltcpGainValueLabel->setText(QString("%1 dB").arg(m_rtltcpGainList.at(val)));
    m_settings.rtltcp.gainIdx = val;
    emit newInputDeviceSettings();
}

void SetupDialog::onRtlTcpIpAddrEditFinished()
{
    if (ui->rtltcpIpAddressEdit->text() != m_settings.rtltcp.tcpAddress)
    {
        ui->connectButton->setVisible(true);
    }
}

void SetupDialog::onRtlTcpPortValueChanged(int val)
{
    if (val != m_settings.rtltcp.tcpPort)
    {
        ui->connectButton->setVisible(true);
    }

}

void SetupDialog::onRtlGainModeToggled(bool checked)
{
    if (checked)
    {
        if (ui->rtlsdrGainModeHw->isChecked())
        {
            m_settings.rtlsdr.gainMode = RtlGainMode::Hardware;
        }
        else if (ui->rtlsdrGainModeSw->isChecked())
        {
            m_settings.rtlsdr.gainMode = RtlGainMode::Software;
        }
        else if (ui->rtlsdrGainModeManual->isChecked())
        {
            m_settings.rtlsdr.gainMode = RtlGainMode::Manual;
        }
        activateRtlSdrControls(true);
        emit newInputDeviceSettings();
    }
}

void SetupDialog::onTcpGainModeToggled(bool checked)
{
    if (checked)
    {
        if (ui->rtltcpGainModeHw->isChecked())
        {
            m_settings.rtltcp.gainMode = RtlGainMode::Hardware;
        }
        else if (ui->rtltcpGainModeSw->isChecked())
        {
            m_settings.rtltcp.gainMode = RtlGainMode::Software;
        }
        else if (ui->rtltcpGainModeManual->isChecked())
        {
            m_settings.rtltcp.gainMode = RtlGainMode::Manual;
        }
        activateRtlTcpControls(true);
        emit newInputDeviceSettings();
    }
}

void SetupDialog::activateRtlSdrControls(bool en)
{
    ui->rtlsdrGainModeGroup->setEnabled(en);
    ui->rtlsdrGainWidget->setEnabled(en && (RtlGainMode::Manual == m_settings.rtlsdr.gainMode));
}

void SetupDialog::activateRtlTcpControls(bool en)
{
    ui->rtltcpGainModeGroup->setEnabled(en);
    ui->rtltcpGainWidget->setEnabled(en && (RtlGainMode::Manual == m_settings.rtltcp.gainMode));
}

void SetupDialog::onRawFileFormatChanged(int idx)
{
    if (static_cast<RawFileInputFormat>(idx) != m_settings.rawfile.format)
    {
        ui->connectButton->setVisible(true);
    }
}

#if HAVE_AIRSPY
void SetupDialog::activateAirspyControls(bool en)
{
    ui->airspyGainModeGroup->setEnabled(en);
    bool sensEna = en && (AirpyGainMode::Sensitivity == m_settings.airspy.gain.mode);
    bool manualEna = en && (AirpyGainMode::Manual == m_settings.airspy.gain.mode);

    // sensitivity controls
    ui->airspySensitivityGain->setEnabled(sensEna);
    ui->airspySensitivityGainLabel->setEnabled(sensEna);
    ui->airspySensitivityGainSlider->setEnabled(sensEna);

    // manual gain controls
    ui->airspyIFGain->setEnabled(manualEna);
    ui->airspyIFGainLabel->setEnabled(manualEna);
    ui->airspyIFGainSlider->setEnabled(manualEna);

    ui->airspyLNAAGCCheckbox->setEnabled(manualEna);
    ui->airspyLNAGain->setEnabled(manualEna && !m_settings.airspy.gain.lnaAgcEna);
    ui->airspyLNAGainLabel->setEnabled(manualEna && !m_settings.airspy.gain.lnaAgcEna);
    ui->airspyLNAGainSlider->setEnabled(manualEna && !m_settings.airspy.gain.lnaAgcEna);

    ui->airspyMixerAGCCheckbox->setEnabled(manualEna);
    ui->airspyMixerGain->setEnabled(manualEna && !m_settings.airspy.gain.mixerAgcEna);
    ui->airspyMixerGainLabel->setEnabled(manualEna && !m_settings.airspy.gain.mixerAgcEna);
    ui->airspyMixerGainSlider->setEnabled(manualEna && !m_settings.airspy.gain.mixerAgcEna);
}

void SetupDialog::onAirspyModeToggled(bool checked)
{
    if (checked)
    {
        if (ui->airspyGainModeHybrid->isChecked())
        {
            m_settings.airspy.gain.mode = AirpyGainMode::Hybrid;
        }
        else if (ui->airspyGainModeSw->isChecked())
        {
            m_settings.airspy.gain.mode = AirpyGainMode::Software;
        }
        else if (ui->airspyGainModeManual->isChecked())
        {
            m_settings.airspy.gain.mode = AirpyGainMode::Manual;
        }
        else if (ui->airspyGainModeSensitivity->isChecked())
        {
            m_settings.airspy.gain.mode = AirpyGainMode::Sensitivity;
        }
        activateAirspyControls(true);
        emit newInputDeviceSettings();
    }
}

void SetupDialog::onAirspySensitivityGainSliderChanged(int val)
{
    ui->airspySensitivityGainLabel->setText(QString::number(val));
    m_settings.airspy.gain.sensitivityGainIdx = val;

    emit newInputDeviceSettings();
}

void SetupDialog::onAirspyIFGainSliderChanged(int val)
{
    ui->airspyIFGainLabel->setText(QString::number(val));
    m_settings.airspy.gain.ifGainIdx = val;
    emit newInputDeviceSettings();
}

void SetupDialog::onAirspyLNAGainSliderChanged(int val)
{
    ui->airspyLNAGainLabel->setText(QString::number(val));
    m_settings.airspy.gain.lnaGainIdx = val;
    emit newInputDeviceSettings();
}

void SetupDialog::onAirspyMixerGainSliderChanged(int val)
{
    ui->airspyMixerGainLabel->setText(QString::number(val));
    m_settings.airspy.gain.mixerGainIdx = val;
    emit newInputDeviceSettings();
}

void SetupDialog::onAirspyLNAAGCstateChanged(int state)
{
    bool ena = (Qt::Unchecked == state);
    ui->airspyLNAGain->setEnabled(ena);
    ui->airspyLNAGainLabel->setEnabled(ena);
    ui->airspyLNAGainSlider->setEnabled(ena);

    m_settings.airspy.gain.lnaAgcEna = !ena;
    emit newInputDeviceSettings();
}

void SetupDialog::onAirspyMixerAGCstateChanged(int state)
{
    bool ena = (Qt::Unchecked == state);
    ui->airspyMixerGain->setEnabled(ena);
    ui->airspyMixerGainLabel->setEnabled(ena);
    ui->airspyMixerGainSlider->setEnabled(ena);

    m_settings.airspy.gain.mixerAgcEna = !ena;
    emit newInputDeviceSettings();
}
#endif // HAVE_AIRSPY

#if HAVE_SOAPYSDR
void SetupDialog::onSoapySdrGainSliderChanged(int val)
{
    ui->soapysdrGainValueLabel->setText(QString("%1 dB").arg(m_soapysdrGainList.at(val)));
    m_settings.soapysdr.gainIdx = val;
    emit newInputDeviceSettings();
}

void SetupDialog::activateSoapySdrControls(bool en)
{
    ui->soapysdrGainModeGroup->setEnabled(en);
    ui->soapysdrGainWidget->setEnabled(en && (SoapyGainMode::Manual == m_settings.soapysdr.gainMode));
}

void SetupDialog::onSoapySdrDevArgsEditFinished()
{
    if (ui->soapysdrDevArgs->text().trimmed() != m_settings.soapysdr.devArgs.trimmed())
    {
        ui->connectButton->setVisible(true);
    }
}

void SetupDialog::onSoapySdrAntennaEditFinished()
{
    if (ui->soapySdrAntenna->text().trimmed() != m_settings.soapysdr.antenna.trimmed())
    {
        ui->connectButton->setVisible(true);
    }
}

void SetupDialog::onSoapySdrChannelEditFinished()
{
    if (ui->soapysdrChannelNum->value() != m_settings.soapysdr.channel)
    {
        ui->connectButton->setVisible(true);
    }
}

void SetupDialog::onSoapySdrGainModeToggled(bool checked)
{
    if (checked)
    {
        if (ui->soapysdrGainModeHw->isChecked())
        {
            m_settings.soapysdr.gainMode = SoapyGainMode::Hardware;
        }
        else if (ui->soapysdrGainModeSw->isChecked())
        {
            m_settings.soapysdr.gainMode = SoapyGainMode::Software;
        }
        else if (ui->soapysdrGainModeManual->isChecked())
        {
            m_settings.soapysdr.gainMode = SoapyGainMode::Manual;
        }
        activateSoapySdrControls(true);
        emit newInputDeviceSettings();
    }
}

#endif // HAVE_SOAPYSDR

void SetupDialog::setStatusLabel()
{
    switch (m_settings.inputDevice)
    {
    case InputDeviceId::RTLSDR:
        ui->statusLabel->setText("RTL SDR device connected");
        break;
    case InputDeviceId::RTLTCP:
        ui->statusLabel->setText("RTL TCP device connected");
        break;
    case InputDeviceId::RARTTCP:
        ui->statusLabel->setText("RART TCP device connected");
        break;
    case InputDeviceId::UNDEFINED:
        ui->statusLabel->setText("<span style=\"color:red\">No device connected</span>");
        break;
    case InputDeviceId::RAWFILE:
        ui->statusLabel->setText("Raw file connected");
        break;
    case InputDeviceId::AIRSPY:
        ui->statusLabel->setText("Airspy device connected");
        break;
    case InputDeviceId::SOAPYSDR:
        ui->statusLabel->setText("Soapy SDR device connected");
        break;
    }
}

void SetupDialog::onExpertMode(bool ena)
{
    int idx = ui->inputCombo->findData(QVariant(static_cast<int>(InputDeviceId::RAWFILE)));
    if (idx >= 0)
    {
        if (!ena)
        {   // remove it
            ui->inputCombo->removeItem(idx);
        }
        return;

    }

    // not found
    if (ena)
    {   // add item
        ui->inputCombo->addItem("Raw file", QVariant(int(InputDeviceId::RAWFILE)));
    }
}

void SetupDialog::onInputChanged(int index)
{
    int inputDeviceInt = ui->inputCombo->itemData(index).toInt();
    ui->deviceOptionsWidget->setCurrentIndex(inputDeviceInt-1);
    adjustSize();

    ui->connectButton->setHidden(m_settings.inputDevice == static_cast<InputDeviceId>(inputDeviceInt));
    if (m_settings.inputDevice != static_cast<InputDeviceId>(inputDeviceInt))
    {   // selected input device does not match current input device
        switch (static_cast<InputDeviceId>(inputDeviceInt))
        {
        case InputDeviceId::RTLSDR:
            activateRtlSdrControls(false);
            break;            
        case InputDeviceId::RTLTCP:
            activateRtlTcpControls(false);
            break;
        case InputDeviceId::RARTTCP:
            break;
        case InputDeviceId::UNDEFINED:
            break;
        case InputDeviceId::RAWFILE:
            break;
        case InputDeviceId::AIRSPY:
#if HAVE_AIRSPY
            activateAirspyControls(false);
#endif
            return;
        case InputDeviceId::SOAPYSDR:
#if HAVE_SOAPYSDR
            activateSoapySdrControls(false);
#endif
            return;
        }
    }
}

void SetupDialog::onOpenFileButtonClicked()
{
    QString dir = QDir::homePath();
    if (!m_rawfilename.isEmpty())
    {
        dir = QFileInfo(m_rawfilename).path();
    }
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open IQ stream"), dir, tr("Binary files (*.bin *.s16 *.u8 *.raw)"));
    if (!fileName.isEmpty())
    {
        m_rawfilename = fileName;
        ui->fileNameLabel->setText(QFileInfo(fileName).fileName());
        ui->fileNameLabel->setToolTip(fileName);
        if (fileName.endsWith(".s16"))
        {
            ui->fileFormatCombo->setCurrentIndex(int(RawFileInputFormat::SAMPLE_FORMAT_S16));
            qDebug() << "Selected file :" << fileName << "SAMPLE_FORMAT_S16";
        }
        else if (fileName.endsWith(".u8"))
        {
            ui->fileFormatCombo->setCurrentIndex(int(RawFileInputFormat::SAMPLE_FORMAT_U8));
            qDebug() << "Selected file :" << fileName << "SAMPLE_FORMAT_U8";
        }
        else
        {
            qDebug() << "Selected file :" << fileName << "Sample format unknown";
        }

        ui->connectButton->setVisible(true);
    }
}

void SetupDialog::resetInputDevice()
{
    switch (m_settings.inputDevice)
    {
    case InputDeviceId::RTLSDR:
        activateRtlSdrControls(false);
        break;
    case InputDeviceId::RTLTCP:
        activateRtlTcpControls(false);
        break;
    case InputDeviceId::RAWFILE:
        break;
    case InputDeviceId::RARTTCP:
    case InputDeviceId::UNDEFINED:
    case InputDeviceId::AIRSPY:
#if HAVE_AIRSPY
        activateAirspyControls(false);
#endif
        break;
    case InputDeviceId::SOAPYSDR:
#if HAVE_SOAPYSDR
        activateSoapySdrControls(false);
#endif
        break;
    }

    m_settings.inputDevice = InputDeviceId::UNDEFINED;
    setStatusLabel();
    ui->connectButton->setVisible(true);
}

void SetupDialog::onAnnouncementClicked()
{   // calculate ena flag
    uint16_t announcementEna = 0;
    for (int a = 0; a < static_cast<int>(DabAnnouncement::Undefined); ++a)
    {
        announcementEna |= (m_announcementCheckBox[a]->isChecked() << a);
    }
    if (m_settings.announcementEna != announcementEna)
    {
        m_settings.announcementEna = announcementEna;
        emit newAnnouncementSettings(announcementEna);
    }
}
