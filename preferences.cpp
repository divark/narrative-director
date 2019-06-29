#include "preferences.h"
#include "ui_preferences.h"

Preferences::Preferences(QWidget *parent, QAudioRecorder* recorder) :
    QDialog(parent),
    ui(new Ui::Preferences)
{
    this->recorder = recorder;
    ui->setupUi(this);
    this->setWindowTitle("Preferences");

    populateFromGlobals();

    //audio devices
    ui->audioDeviceBox->addItem(tr("Default"), QVariant(QString()));
    for (auto &device: recorder->audioInputs()) {
        ui->audioDeviceBox->addItem(device, QVariant(device));
    }

    //audio codecs
    ui->audioCodecBox->addItem(tr("Default"), QVariant(QString()));
    for (auto &codecName: recorder->supportedAudioCodecs()) {
        ui->audioCodecBox->addItem(codecName, QVariant(codecName));
    }

    //containers
    ui->containerBox->addItem(tr("Default"), QVariant(QString()));
    for (auto &containerName: recorder->supportedContainers()) {
        ui->containerBox->addItem(containerName, QVariant(containerName));
    }

    //sample rate
    ui->sampleRateBox->addItem(tr("Default"), QVariant(0));
    for (int sampleRate: recorder->supportedAudioSampleRates()) {
        ui->sampleRateBox->addItem(QString::number(sampleRate), QVariant(
                sampleRate));
    }

    //channels
    ui->channelsBox->addItem(tr("Default"), QVariant(-1));
    ui->channelsBox->addItem(QStringLiteral("1"), QVariant(1));
    ui->channelsBox->addItem(QStringLiteral("2"), QVariant(2));
    ui->channelsBox->addItem(QStringLiteral("4"), QVariant(4));
}

Preferences::~Preferences()
{
    delete ui;
}

static QVariant boxValue(const QComboBox *box)
{
    int idx = box->currentIndex();
    if (idx == -1)
        return QVariant();

    return box->itemData(idx);
}

void Preferences::on_buttonBox_accepted()
{
    recorder->setAudioInput(boxValue(ui->audioDeviceBox).toString());

    auto selectedCodec = boxValue(ui->audioCodecBox).toString();
    auto selectedSampleRate = boxValue(ui->sampleRateBox).toInt();
    auto selectedChannelCount = boxValue(ui->channelsBox).toInt();
    auto selectedContainer = boxValue(ui->containerBox).toString();

    QAudioEncoderSettings settings;
    settings.setCodec(selectedCodec);
    globalSettings.setValue("preferences/codec", selectedCodec);
    settings.setSampleRate(selectedSampleRate);
    globalSettings.setValue("preferences/sampleRate", selectedSampleRate);
    settings.setChannelCount(selectedChannelCount);
    globalSettings.setValue("preferences/channelCount", selectedChannelCount);
    settings.setQuality(QMultimedia::VeryHighQuality);
    globalSettings.setValue("preferences/container", selectedContainer);

    recorder->setEncodingSettings(settings, QVideoEncoderSettings(), selectedContainer);
}

void Preferences::populateFromGlobals() {
    QAudioEncoderSettings settings;

    if(globalSettings.contains("preferences/codec"))
        settings.setCodec(globalSettings.value("preferences/codec").toString());
    if(globalSettings.contains("preferences/sampleRate"))
        settings.setSampleRate(globalSettings.value("preferences/sampleRate").toInt());
    if(globalSettings.contains("preferences/channelCount"))
        settings.setChannelCount(globalSettings.value("preferences/channelCount").toInt());

    settings.setQuality(QMultimedia::VeryHighQuality);
    QString container = "";
    if(globalSettings.contains("preferences/container"))
        container = globalSettings.value("preferences/container").toString();

    recorder->setEncodingSettings(settings, QVideoEncoderSettings(), container);
}
