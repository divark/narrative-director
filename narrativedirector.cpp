#include "narrativedirector.h"
#include "ui_narrativedirector.h"

NarrativeDirector::NarrativeDirector(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::NarrativeDirector)
{
    ui->setupUi(this);

    audioRecorder = new QAudioRecorder(this);
    audioPlayer = new QMediaPlayer(this);
    preferences = new Preferences(this, audioRecorder);

    recordingPosition = QTime(0, 0, 0, 0);
    recordingDuration = QTime(0, 0, 0, 0);

    connect(audioRecorder, &QAudioRecorder::stateChanged, this, &NarrativeDirector::onARStateChanged);
    connect(audioRecorder, &QAudioRecorder::durationChanged, this, &NarrativeDirector::updateAEnd);

    connect(audioPlayer, &QMediaPlayer::stateChanged, this, &NarrativeDirector::onMPStateChanged);
    connect(audioPlayer, &QMediaPlayer::mediaStatusChanged, this, &NarrativeDirector::onMPMediaStatusChanged);
    connect(audioPlayer, &QMediaPlayer::positionChanged, this, &NarrativeDirector::updateAProgress);
}

NarrativeDirector::~NarrativeDirector()
{
    if(narrativeFile.isOpen())
        narrativeFile.close();

    delete audioRecorder;
    delete audioPlayer;
    delete ui;
    delete preferences;
}

void NarrativeDirector::updateTimeLbl() {
    ui->timeLbl->setText(recordingPosition.toString() + '/' + recordingDuration.toString());
}

void NarrativeDirector::updateAProgress(int duration) {
    if (audioRecorder->error() != QAudioRecorder::NoError
            || audioPlayer->error() != QMediaPlayer::NoError
            || duration < 1000)
        return;

    QTime newTime(0, 0, 0, 0);
    recordingPosition = newTime.addMSecs(duration);

    if(recordingPosition > recordingDuration)
        recordingDuration = recordingPosition;

    updateTimeLbl();
}

void NarrativeDirector::updateAEnd(int duration) {
    if (audioRecorder->error() != QAudioRecorder::NoError
            || audioPlayer->error() != QMediaPlayer::NoError
            || duration < 1000)
        return;

    QTime newTime(0, 0, 0, 0);
    recordingDuration = newTime.addMSecs(duration);

    updateTimeLbl();
}

void NarrativeDirector::on_recordBtn_clicked()
{
    if(QFileInfo(recordingLocation.path()).exists()) {
        QFile recordingFile(recordingLocation.path());

        recordingFile.remove();
    }

    updateRecordingLocation();

    audioRecorder->setOutputLocation(recordingLocation);
    audioRecorder->record();
}

void NarrativeDirector::on_playBtn_clicked()
{
    //Audio player checks
    if(audioPlayer->state() == QMediaPlayer::PausedState ||
            audioPlayer->state() == QMediaPlayer::StoppedState) {
        audioPlayer->play();
    } else if(audioPlayer->state() == QMediaPlayer::PlayingState) {
        audioPlayer->pause();
    }

    //Audio recording check
    if(audioRecorder->state() == QAudioRecorder::RecordingState) {
        audioRecorder->pause();
    }
}

void NarrativeDirector::on_stopBtn_clicked()
{
    if(audioRecorder->state() == QAudioRecorder::RecordingState) {
        audioRecorder->stop();

        auto outputLocation = audioRecorder->outputLocation();
        audioPlayer->setMedia(outputLocation);
        audioExtension = outputLocation.fileName().right(4);
        return;
    }

    //I must be playing audio otherwise, so
    audioPlayer->stop();
    recordingPosition = QTime(0, 0, 0, 0);
    updateTimeLbl();
}


void NarrativeDirector::on_backBtn_clicked()
{
    if(prgNum - 1 < 0) return;

    recordingPosition = QTime(0, 0, 0, 0);
    updateTimeLbl();

    try {
        changeParagraphLbl(prgNum - 1);
        prgNum--;
        updateRecordingLocation();

        if(QFileInfo(recordingLocation.path()).exists())
            audioPlayer->setMedia(recordingLocation);
        else
            audioPlayer->setMedia(nullptr);
    } catch(std::string &myError) {
        qDebug() << QString::fromStdString(myError);
    }
}

void NarrativeDirector::on_nextBtn_clicked()
{
    if(paragraphs.length() == 0) return;

    recordingPosition = QTime(0, 0, 0, 0);
    updateTimeLbl();

    try {
        changeParagraphLbl(prgNum + 1);
        prgNum++;
        updateRecordingLocation();

        if(QFileInfo(recordingLocation.path()).exists())
            audioPlayer->setMedia(recordingLocation);
        else
            audioPlayer->setMedia(nullptr);
    } catch(std::string &myError) {
        qDebug() << QString::fromStdString(myError);
    }
}

void NarrativeDirector::on_actionNew_triggered()
{
    if(!promptIfNotSaved())
        return;

    QString fileName = QFileDialog::getOpenFileName(
                this,
                tr("Open Text File"),
                QDir::currentPath(),
                tr("Text files (*.txt)")
    );

    if(fileName.isNull()) return;
    if(narrativeFile.isOpen())
        narrativeFile.close();

    narrativeFile.setFileName(fileName);
    if(!narrativeFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    prgNum = 0;
    filePos = 0;
    narrativeInput.setDevice(&narrativeFile);

    changeParagraphLbl(prgNum);
    updateRecordingLocation();

    ui->recordBtn->setEnabled(true);
}

void NarrativeDirector::on_actionOpen_triggered()
{
    if(!promptIfNotSaved())
        return;

    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Open Project",
        QDir::currentPath(),
        "Narrative Director Project Files (*.ndp)"
    );

    if(fileName.isNull()) return;

    loadFromProjectFile(fileName);

    changeParagraphLbl(prgNum);
    updateRecordingLocation();
    if(QFileInfo(recordingLocation.path()).exists())
        audioPlayer->setMedia(recordingLocation);

    ui->recordBtn->setEnabled(true);
}

void NarrativeDirector::onARStateChanged(QAudioRecorder::State state)
{
    switch (state) {
    case QAudioRecorder::RecordingState:
        ui->recordBtn->setEnabled(false);
        ui->playBtn->setEnabled(true);
        ui->stopBtn->setEnabled(true);
        ui->backBtn->setEnabled(false);
        ui->nextBtn->setEnabled(false);

        ui->playBtn->setText(tr("Pause"));
        hasChanged = true;
        break;
    case QAudioRecorder::PausedState:
        ui->recordBtn->setEnabled(true);
        ui->playBtn->setEnabled(false);
        break;
    case QAudioRecorder::StoppedState:
        ui->recordBtn->setEnabled(true);
        ui->backBtn->setEnabled(true);
        ui->nextBtn->setEnabled(true);
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
        ui->backBtn->setEnabled(false);
        ui->nextBtn->setEnabled(false);
        break;
    case QMediaPlayer::PausedState:
        ui->playBtn->setText(tr("Play"));
        break;
    case QMediaPlayer::StoppedState:
        ui->playBtn->setText(tr("Play"));
        ui->recordBtn->setEnabled(true);
        ui->backBtn->setEnabled(true);
        ui->nextBtn->setEnabled(true);
        break;
    }
}

void NarrativeDirector::onMPMediaStatusChanged(QMediaPlayer::MediaStatus mediaStatus) {
    switch (mediaStatus) {
    case QMediaPlayer::LoadedMedia:
        recordingDuration = QTime(0, 0, 0, 0).addMSecs(audioPlayer->duration());

        ui->playBtn->setEnabled(true);
        ui->stopBtn->setEnabled(true);
        updateTimeLbl();
        break;
    case QMediaPlayer::InvalidMedia:
    case QMediaPlayer::UnknownMediaStatus:
    case QMediaPlayer::NoMedia:
        recordingDuration = QTime(0, 0, 0, 0);

        ui->playBtn->setEnabled(false);
        ui->stopBtn->setEnabled(false);
        updateTimeLbl();
        qDebug() << recordingLocation.path() << " Cannot be played.";
    }
}

void NarrativeDirector::changeParagraphLbl(int prgIndex) {
    std::stringstream prgStream;

    prgStream << "Paragraph " << prgIndex + 1;
    ui->prgText->setPlainText(getParagraph(prgIndex));
    ui->prgLbl->setText(QString::fromStdString(prgStream.str()));
}

QString NarrativeDirector::getParagraphFromFile(qint64 location) {
    QString myParagraph = "";

    for(int i = 0; i < 4; i++) {
        QString sentence = getSentenceFromFile(location);

        if(sentence.isEmpty()) break;
        myParagraph += sentence;
    }
    filePos = location;

    return myParagraph.simplified();
}

QString NarrativeDirector::getSentenceFromFile(qint64& location) {
    QString sentence = "";

    while(1) {
        if(!narrativeInput.seek(location)) break;

        QChar currentLetter = narrativeInput.read(1).front();
        if(narrativeInput.atEnd()) break;
        sentence.append(currentLetter);

        location = narrativeInput.pos();
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
        if(!narrativeInput.seek(location)) break;

        QChar currentLetter = narrativeInput.read(1).front();
        if(narrativeInput.atEnd()) break;

        if(!isEndOfSentence(currentLetter)) {
            if(currentLetter == '"' or currentLetter == '\''
                    or currentLetter == "”" or currentLetter == '`') {
                sentence.append(currentLetter);
                location = narrativeInput.pos();
            }
            break;
        }

        sentence.append(currentLetter);
        location = narrativeInput.pos();
    }
}

QString NarrativeDirector::getParagraph(int paragraphNum) {
    std::pair<qint64, QString> prgEntry;

    if(paragraphNum < paragraphs.length()) {
        prgEntry = paragraphs[paragraphNum];

        if(!prgEntry.second.isNull())
            return prgEntry.second;

        return getParagraphFromFile(prgEntry.first);
    }

    if(narrativeInput.atEnd()) throw std::string("File at end.");

    prgEntry.first = filePos;
    QString paragraph = getParagraphFromFile(filePos);
    prgEntry.second = paragraph;

    paragraphs.push_back(prgEntry);
    hasChanged = true;

    return paragraph;
}

void NarrativeDirector::saveToProjectFile(QString filePath) {
    QFile outputProjFile(filePath);
    if(!outputProjFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream fileOutput(&outputProjFile);
    for(auto prgPair : paragraphs)
        fileOutput << prgPair.first << ",";
    fileOutput << '\n' << flush;
    fileOutput << prgNum << '\n' << flush;
    fileOutput << narrativeFile.fileName() << '\n' << flush;
    fileOutput << audioExtension << '\n' << flush;

    outputProjFile.close();
}

void NarrativeDirector::loadFromProjectFile(QString filePath) {
    QFile openedProjectFile(filePath);
    if(!openedProjectFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream prjInput(&openedProjectFile);

    //Known paragraph locations
    QString prgStarts = prjInput.readLine();
    std::istringstream prgStream(prgStarts.toStdString());
    std::string foundPrgLoc;
    while(std::getline(prgStream, foundPrgLoc, ',')) {
        std::pair<int, QString> prgEntry;
        prgEntry.first = QString::fromStdString(foundPrgLoc).toInt();

        paragraphs.push_back(prgEntry);
    }

    //Paragraph user was last on.
    prgNum = prjInput.readLine().toInt();

    //Text file being read for narration.
    narrativeFile.setFileName(prjInput.readLine());
    if(!narrativeFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    this->narrativeInput.setDevice(&narrativeFile);

    //File last modified check.
//    fileLastModifed = QDateTime::fromString(prjInput.readLine());
//    QDateTime currentLastModified = QFileInfo(narrativeFile).lastModified();

//    if(QFileInfo(narrativeFile).lastModified() != fileLastModifed) {
//        std::string warningMessage = narrativeFile.fileName().toStdString();
//        warningMessage.append(" has been modified.");
//        QMessageBox::information(
//                    this,
//                    tr("NarrativeDirector"),
//                    tr(warningMessage.c_str())
//        );

//        fileLastModifed = currentLastModified;
//    }

    //Audio extension for parts
    audioExtension = prjInput.readLine();

    openedProjectFile.close();
}

void NarrativeDirector::on_actionSave_As_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save Project",
        QDir::currentPath(),
        "Narrative Director Project Files (*.ndp)"
    );

    if(fileName.isNull()) return;

    saveToProjectFile(fileName);
    currentProjectFile = fileName;
    hasChanged = false;
}

void NarrativeDirector::on_actionSave_triggered()
{
    if(currentProjectFile.isNull()) {
        on_actionSave_As_triggered();
        return;
    }

    saveToProjectFile(currentProjectFile);
    hasChanged = false;
}

void NarrativeDirector::closeEvent(QCloseEvent *event) {
    if(hasChanged) {
        QMessageBox::StandardButton exitBtn = QMessageBox::question(
                    this, "NarrativeDirector",
                    tr("You have unsaved changes. Would you like to save?\n"),
                    QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes);

        if(exitBtn == QMessageBox::Yes)
            on_actionSave_triggered();
        else if(exitBtn == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }

    event->accept();
}

bool NarrativeDirector::promptIfNotSaved() {
    if(hasChanged) {
        QMessageBox::StandardButton exitBtn = QMessageBox::question(
                    this, "NarrativeDirector",
                    tr("You have unsaved changes. Would you like to save?\n"),
                    QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes);

        if(exitBtn == QMessageBox::Yes) {
            on_actionSave_triggered();
        } else if(exitBtn == QMessageBox::Cancel) {
            return false;
        }
    }

    narrativeFile.close();
    narrativeInput.flush();
    narrativeInput.reset();
    paragraphs.clear();
    return true;
}

void NarrativeDirector::on_actionPreferences_triggered()
{
    preferences->show();
}

void NarrativeDirector::updateRecordingLocation() {    
    QString recordingFileDirName = QFileInfo(narrativeFile.fileName()).fileName();
    recordingFileDirName.chop(4);

    QString recordingPath = QStandardPaths::writableLocation(QStandardPaths::MusicLocation)
            + QDir::separator() + recordingFileDirName;

    if(!QDir(recordingPath).exists())
        QDir().mkdir(recordingPath);

    QString recordingFileName = "part" + QString::number(prgNum)
            + audioExtension;
    auto myThing = QStandardPaths::MusicLocation;
    recordingLocation = QUrl::fromUserInput(recordingFileName,
                                            recordingPath,
                                            QUrl::UserInputResolutionOption::AssumeLocalFile);
}
