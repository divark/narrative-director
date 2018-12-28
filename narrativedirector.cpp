#include "narrativedirector.h"
#include "ui_narrativedirector.h"

NarrativeDirector::NarrativeDirector(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::NarrativeDirector)
{
    ui->setupUi(this);

    audioRecorder = new QAudioRecorder(this);
    audioPlayer = new QMediaPlayer(this);

    connect(audioRecorder, &QAudioRecorder::stateChanged, this, &NarrativeDirector::onARStateChanged);
    //connect(audioRecorder, QOverload<QMediaRecorder::Error>::of(&QAudioRecorder::error), this,
    //        &NarrativeDirector::displayErrorMessage);

    connect(audioPlayer, &QMediaPlayer::stateChanged, this, &NarrativeDirector::onMPStateChanged);
    //connect(audioPlayer, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this,
    //        &NarrativeDirector::displayErrorMessage);
}

NarrativeDirector::~NarrativeDirector()
{
    delete audioRecorder;
    delete audioPlayer;
    delete ui;
}

void NarrativeDirector::on_recordBtn_clicked()
{
    QAudioEncoderSettings settings;
    settings.setCodec("audio/wav");
    settings.setQuality(QMultimedia::VeryHighQuality);

    audioRecorder->setEncodingSettings(settings);
    audioRecorder->setAudioInput(audioRecorder->audioInput());
    audioRecorder->record();
}

void NarrativeDirector::on_playBtn_clicked()
{
    if(ui->playBtn->text().compare("Play") == 0) {
        audioPlayer->play();
        qDebug() << audioPlayer->state();
        return;
    }

    if(ui->recordBtn->isEnabled())
        audioRecorder->pause();
    else
        audioPlayer->pause();
}

void NarrativeDirector::on_backBtn_clicked()
{
    QString recordFile = "part" + QString::number(prgIndex - 1) + ".wav";

    if(prgIndex > 0) {
        changeParagraphLbl(--prgIndex);
        audioRecorder->setOutputLocation(QUrl::fromLocalFile(recordFile));
        audioPlayer->setMedia(QUrl::fromLocalFile(recordFile));
    }
}

void NarrativeDirector::on_nextBtn_clicked()
{
    QString recordFile = "part" + QString::number(prgIndex + 1) + ".wav";

    if(prgIndex < paragraphs.length() - 1) {
        changeParagraphLbl(++prgIndex);
        audioRecorder->setOutputLocation(QUrl::fromLocalFile(recordFile));
        audioPlayer->setMedia(QUrl::fromLocalFile(recordFile));
        return;
    }

    if(currentFile.isOpen()) {
        QString paragraph = getParagraphFromFile(filePos);

        if(paragraph.isEmpty()) {
            currentFile.close();
            return;
        }

        paragraphs.append(paragraph);
        changeParagraphLbl(++prgIndex);
        audioRecorder->setOutputLocation(QUrl::fromLocalFile(recordFile));
        audioPlayer->setMedia(QUrl::fromLocalFile(recordFile));
    }
}

void NarrativeDirector::on_actionNew_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(
                this,
                tr("Open Text File"),
                QDir::currentPath(),
                tr("Text files (*.txt)")
    );

    if(fileName.isNull()) return;
    currentFile.setFileName(fileName);
    if(!currentFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    prgIndex = 0;
    filePos = 0;
    paragraphs.clear();

    fileInput.setDevice(&currentFile);
    QString paragraph = getParagraphFromFile(filePos);
    QString recordFile = "part" + QString::number(prgIndex);

    if(paragraph.isEmpty()) return;

    paragraphs.push_back(paragraph);
    changeParagraphLbl(prgIndex);
    audioRecorder->setOutputLocation(QUrl::fromLocalFile(recordFile));

    ui->recordBtn->setEnabled(true);
}

void NarrativeDirector::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Open Project",
        QDir::currentPath(),
        "Narrative Director Project Files (*.ndp)"
    );

    if(fileName.isNull()) return;
}

void NarrativeDirector::onARStateChanged(QAudioRecorder::State state)
{
    switch (state) {
    case QAudioRecorder::RecordingState:
        ui->recordBtn->setEnabled(false);
        ui->playBtn->setEnabled(true);
        ui->stopBtn->setEnabled(true);
        ui->playBtn->setText(tr("Pause"));
        break;
    case QAudioRecorder::PausedState:
        ui->recordBtn->setEnabled(true);
        break;
    case QAudioRecorder::StoppedState:
        ui->recordBtn->setEnabled(true);
        ui->playBtn->setText(tr("Play"));
        break;
    }
}

void NarrativeDirector::onMPStateChanged(QMediaPlayer::State state)
{
    switch (state) {
    case QMediaPlayer::PlayingState:
        ui->playBtn->setText(tr("Pause"));
        ui->recordBtn->setEnabled(false);
        break;
    case QMediaPlayer::PausedState:
        ui->playBtn->setText(tr("Play"));
        break;
    case QMediaPlayer::StoppedState:
        ui->playBtn->setText(tr("Play"));
        ui->recordBtn->setEnabled(true);
        break;
    }
}

void NarrativeDirector::changeParagraphLbl(int prgIndex) {
    if(prgIndex > paragraphs.length() - 1) return;

    std::stringstream prgStream;

    prgStream << "Paragraph " << prgIndex + 1;
    ui->prgLbl->setText(QString::fromStdString(prgStream.str()));
    ui->prgText->setPlainText(paragraphs[prgIndex]);
}

QString NarrativeDirector::getParagraphFromFile(qint64& location) {
    QString myParagraph = "";

    for(int i = 0; i < 4; i++) {
        QString sentence = getSentenceFromFile(location);

        if(sentence.isEmpty()) break;
        myParagraph += sentence;
    }

    return myParagraph.simplified();
}

QString NarrativeDirector::getSentenceFromFile(qint64& location) {
    QString sentence = "";

    while(1) {
        if(!fileInput.seek(location)) {
            currentFile.close();
            break;
        }

        QChar currentLetter = fileInput.read(1).front();
        if(fileInput.atEnd()) break;
        sentence.append(currentLetter);

        location = fileInput.pos();
        if(isEndOfSentence(currentLetter)) {
            getToStartOfNextSentence(sentence, location);
            break;
        }
    }

    return sentence;
}

bool NarrativeDirector::isEndOfSentence(QChar letter) {
    return (letter == '!' || letter == '?' || letter == '.');
}

void NarrativeDirector::getToStartOfNextSentence(QString& sentence, qint64& location) {
    while(1) {
        if(!fileInput.seek(location)) {
            currentFile.close();
            break;
        }

        QChar currentLetter = fileInput.read(1).front();
        if(fileInput.atEnd()) break;
        if(!isEndOfSentence(currentLetter)) {
            if(currentLetter == '"' or currentLetter == '\''
                    or currentLetter == "”" or currentLetter == '`') {
                sentence.append(currentLetter);
                location = fileInput.pos();
            }
            break;
        }

        sentence.append(currentLetter);
        location = fileInput.pos();
    }
}

void NarrativeDirector::on_stopBtn_clicked()
{
    if(!ui->recordBtn->isEnabled()) {
        audioRecorder->stop();
        ui->recordBtn->setEnabled(true);
        ui->playBtn->setText("Play");
        return;
    }

    audioPlayer->stop();
    ui->playBtn->setText("Play");
}
