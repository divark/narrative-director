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

    currentTime = QTime(0, 0, 0, 0);
    audioFileDuration = QTime(0, 0, 0, 0);

    connect(audioRecorder, &QAudioRecorder::stateChanged, this, &NarrativeDirector::onARStateChanged);
    connect(audioPlayer, &QMediaPlayer::positionChanged, this, &NarrativeDirector::updateProgress);
    connect(audioRecorder, &QAudioRecorder::durationChanged, this, &NarrativeDirector::updateProgress);

    //connect(audioRecorder, QOverload<QMediaRecorder::Error>::of(&QAudioRecorder::error), this,
    //        &NarrativeDirector::displayErrorMessage);

    connect(audioPlayer, &QMediaPlayer::stateChanged, this, &NarrativeDirector::onMPStateChanged);
    //connect(audioPlayer, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this,
    //        &NarrativeDirector::displayErrorMessage);
}

NarrativeDirector::~NarrativeDirector()
{
    if(currentFile.isOpen())
        currentFile.close();

    delete audioRecorder;
    delete audioPlayer;
    delete ui;
    delete preferences;
}

void NarrativeDirector::updateTimeLbl() {
    ui->timeLbl->setText(currentTime.toString() + '/' + audioFileDuration.toString());
}

void NarrativeDirector::updateProgress(int duration) {
    if (audioPlayer->error() != QMediaPlayer::NoError ||
            audioRecorder->error() != QAudioRecorder::NoError
            || duration < 1000)
        return;

    QTime newTime(0, 0, 0, 0);
    currentTime = newTime.addMSecs(duration);

    if(currentTime > audioFileDuration)
        audioFileDuration = currentTime;

    updateTimeLbl();
    //ui->statusbar->showMessage(tr("Recorded %1 sec").arg(duration / 1000));
}

void NarrativeDirector::on_recordBtn_clicked()
{
    audioRecorder->record();
}

void NarrativeDirector::on_playBtn_clicked()
{
    if(ui->playBtn->text().compare("Play") == 0) {
        if(audioPlayer->state() != QMediaPlayer::PausedState) {
            QTime newTime(0, 0, 0, 0);
            currentTime = newTime;
            updateTimeLbl();
        }

        audioPlayer->play();
        return;
    }

    if(audioRecorder->state() == QAudioRecorder::RecordingState)
        audioRecorder->pause();
    else
        audioPlayer->pause();
}

void NarrativeDirector::on_backBtn_clicked()
{
    if(prgNum - 1 < 0) return;

    try {
        changeParagraphLbl(prgNum - 1);
        changeRecordFile(prgNum - 1);
        prgNum--;
    } catch(std::string &myError) {
        qDebug() << QString::fromStdString(myError);
    }
}

void NarrativeDirector::on_nextBtn_clicked()
{
    if(paragraphs.length() == 0) return;

    try {
        changeParagraphLbl(prgNum + 1);
        changeRecordFile(prgNum + 1);
        prgNum++;
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
    if(currentFile.isOpen())
        currentFile.close();

    currentFile.setFileName(fileName);
    if(!currentFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    setPartsDir(fileName);

    prgNum = 0;
    filePos = 0;
    fileLastModifed = QFileInfo(currentFile).lastModified();
    fileInput.setDevice(&currentFile);

    changeParagraphLbl(prgNum);
    changeRecordFile(prgNum);

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
    changeRecordFile(prgNum);

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
        if(!fileInput.seek(location)) break;

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
        if(!fileInput.seek(location)) break;

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
    if(audioRecorder->state() == QAudioRecorder::RecordingState ||
            audioRecorder->state() == QAudioRecorder::PausedState) {
        audioRecorder->stop();
        audioExtension = audioRecorder->outputLocation().fileName().right(4);
        changeRecordFile(prgNum);
        return;
    }

    audioPlayer->stop();
}

QString NarrativeDirector::getParagraph(int paragraphNum) {
    std::pair<qint64, QString> prgEntry;

    if(paragraphNum < paragraphs.length()) {
        prgEntry = paragraphs[paragraphNum];

        if(!prgEntry.second.isNull())
            return prgEntry.second;

        return getParagraphFromFile(prgEntry.first);
    }

    if(fileInput.atEnd()) throw std::string("File at end.");

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
    fileOutput << currentFile.fileName() << '\n' << flush;
    fileOutput << fileLastModifed.toString() << '\n' << flush;
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
    currentFile.setFileName(prjInput.readLine());
    if(!currentFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    this->fileInput.setDevice(&currentFile);

    //File last modified check.
    fileLastModifed = QDateTime::fromString(prjInput.readLine());
    QDateTime currentLastModified = QFileInfo(currentFile).lastModified();

    if(QFileInfo(currentFile).lastModified() != fileLastModifed) {
        std::string warningMessage = currentFile.fileName().toStdString();
        warningMessage.append(" has been modified.");
        QMessageBox::information(
                    this,
                    tr("NarrativeDirector"),
                    tr(warningMessage.c_str())
        );

        fileLastModifed = currentLastModified;
    }

    //Audio extension for parts
    audioExtension = prjInput.readLine();

    //Parts directory initilization
    setPartsDir(currentFile.fileName());

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

    currentFile.close();
    fileInput.flush();
    fileInput.reset();
    paragraphs.clear();
    return true;
}

void NarrativeDirector::on_actionPreferences_triggered()
{
    preferences->show();
}

void NarrativeDirector::changeRecordFile(int partNum) {
    currentTime = QTime(0, 0, 0, 0);
    audioFileDuration = QTime(0, 0, 0, 0);

    recordFile = partsLocation.path() +
            QDir::separator() + "part" + QString::number(partNum) + audioExtension;

    auto localFile = QUrl::fromLocalFile(recordFile);
    audioRecorder->setOutputLocation(localFile);
    QFileInfo fileCheck(localFile.path());

    if(fileCheck.exists() && fileCheck.isFile()) {
        audioPlayer->setMedia(localFile);
        auto audioMiliDuration = audioPlayer->duration();
        audioFileDuration = QTime(0, 0, 0, 0).addMSecs(audioMiliDuration);

        ui->playBtn->setEnabled(true);
        ui->stopBtn->setEnabled(true);
    } else {
        ui->playBtn->setEnabled(false);
        ui->stopBtn->setEnabled(false);
    }

    updateTimeLbl();
}

void NarrativeDirector::setPartsDir(QString fileName) {
    fileName.chop(4);

    partsLocation.setPath(fileName);
    if(!partsLocation.exists())
        partsLocation.mkpath(".");
}
