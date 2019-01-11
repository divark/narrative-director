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
    if (duration < 1000)
        return;

    QTime newTime(0, 0, 0, 0);
    recordingPosition = newTime.addMSecs(duration);

    if(recordingPosition > recordingDuration)
        recordingDuration = recordingPosition;

    updateTimeLbl();
}

void NarrativeDirector::updateAEnd(int duration) {
    if (duration < 1000)
        return;

    QTime newTime(0, 0, 0, 0);
    recordingDuration = newTime.addMSecs(duration);

    updateTimeLbl();
}

void NarrativeDirector::on_recordBtn_clicked()
{
#ifdef _WIN32
    auto filePath = recordingLocation.toLocalFile().toStdString().c_str();
    auto fileAttributes = GetFileAttributesA(filePath);

    if(fileAttributes != INVALID_FILE_ATTRIBUTES) {
        DeleteFileA(filePath);
        audioPlayer->setMedia(nullptr);
    }
#else
    if(QFileInfo(recordingLocation.path()).exists()) {
        QFile recordingFile(recordingLocation.toLocalFile());

        recordingFile.remove();
    }
#endif
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
        updatePlayerLocation();
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
        updatePlayerLocation();
    } catch(std::string &myError) {
        qDebug() << QString::fromStdString(myError);
    }
}

void NarrativeDirector::on_actionNew_triggered()
{
    if(!promptIfNotSaved())
        return;

    currentProjectFile = nullptr;
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
    paragraphs.clear();
    prgNumTotal = getNumPrgs();

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

    paragraphs.clear();
    paragraphs.squeeze();
    loadFromProjectFile(fileName);

    changeParagraphLbl(prgNum);
    updateRecordingLocation();
    updatePlayerLocation();

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
        if(audioRecorder->state() == QAudioRecorder::RecordingState) return;
        recordingDuration = QTime(0, 0, 0, 0);

        ui->playBtn->setEnabled(false);
        ui->stopBtn->setEnabled(false);
        updateTimeLbl();
        break;
    default:
        break;
    }
}

void NarrativeDirector::changeParagraphLbl(int prgIndex) {
    std::stringstream prgStream;
    QString paragraph = getParagraph(prgIndex);
    if(ui->actionSimplify->isChecked())
        paragraph = paragraph.simplified();

    prgStream << "Paragraph " << prgIndex + 1 <<
                 "/" << prgNumTotal;
    ui->prgText->setPlainText(paragraph);
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

    return myParagraph.trimmed();
}

QString NarrativeDirector::getSentenceFromFile(qint64& location) {
    QString sentence = "";

    narrativeInput.seek(location);
    while(!narrativeInput.atEnd()) {
        QChar currentLetter = narrativeInput.read(1).front();

        sentence.append(currentLetter);
        location = narrativeInput.pos();

        if(isEndOfSentence(currentLetter)) {
            appendUntilNextSentence(sentence, location);
            break;
        }
    }

    return sentence;
}

bool NarrativeDirector::isEndOfSentence(QChar letter) {
    return letter == '!' || letter == '?' || letter == '.';
}

bool NarrativeDirector::isEndOfQuote(QChar letter) {
    return letter == '"' or letter == '\''
              or letter == "”" or letter == '`';
}

void NarrativeDirector::appendUntilNextSentence(QString& sentence, qint64& location) {
    narrativeInput.seek(location);

    while(!narrativeInput.atEnd()) {
        QChar currentLetter = narrativeInput.read(1).front();

        if(isEndOfQuote(currentLetter) || !isEndOfSentence(currentLetter)) {
            sentence.append(currentLetter);
            location = narrativeInput.pos();
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

    return paragraph.trimmed();
}

void NarrativeDirector::saveToProjectFile(QString filePath) {
    QFile outputProjFile(filePath);
    if(!outputProjFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream fileOutput(&outputProjFile);
    fileOutput << prgNumTotal << '\n' << flush;
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

    //Number of paragraphs total.
    prgNumTotal = prjInput.readLine().toUInt();
    //paragraphs.reserve(prjInput.readLine().toInt());

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

    //Audio extension for parts
    audioExtension = prjInput.readLine();

    //Last, but not least, the project file.
    currentProjectFile = filePath;

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
    QString recordingPath = getRecordingPath();

    if(!QDir(recordingPath).exists())
        QDir().mkdir(recordingPath);

    QString recordingFileName = "part" + QString::number(prgNum)
            + audioExtension;

    recordingLocation = QUrl::fromUserInput(recordingFileName,
                                            recordingPath,
                                            QUrl::UserInputResolutionOption::AssumeLocalFile);
}

void NarrativeDirector::updatePlayerLocation() {
#ifdef _WIN32
    auto recordingPath = recordingLocation.toLocalFile();
#else
    auto recordingPath = recordingLocation.path();
#endif

    if(QFileInfo(recordingPath).exists())
        audioPlayer->setMedia(recordingLocation);
    else
        audioPlayer->setMedia(nullptr);
}

void NarrativeDirector::on_actionExport_Parts_File_triggered()
{
    if(paragraphs.length() == 0) {
        showErrorMsg("There are no parts files to record.");
        return;
    }

    QString recordingPath = getRecordingPath();

    if(!QDir(recordingPath).exists() || !QFileInfo(recordingPath).isDir()) {
        showErrorMsg("Recording directory not present.");
        return;
    }

    QFile partsFile(recordingPath + "/" + "parts-list.txt");
    if(!partsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        showErrorMsg("File error for parts-list.txt creation");
        return;
    }

    QTextStream fileOutput(&partsFile);
    for(int i = 0; i < paragraphs.length(); i++) {
        QString recordingFileName = "part" + QString::number(i)
                + audioExtension;

        fileOutput << "file " << recordingPath << "/"
                   << recordingFileName << '\n' << flush;
    }

    QMessageBox donePrompt;
    donePrompt.information(this, "Success", "parts-list.txt created successfully.");
    donePrompt.show();

    partsFile.close();
}


void NarrativeDirector::showErrorMsg(QString errorMsg) {
    QMessageBox errorPrompt;
    errorPrompt.critical(this, "Error", errorMsg);
    errorPrompt.show();
}

void NarrativeDirector::on_actionAbout_Narrative_Director_triggered()
{
    QMessageBox aboutPrompt;
    QString aboutText = "Narrative Director is a Qt program written to assist in audio recording."
                        " It guides the narrator by recording in parts by paragraphs."
                        " This program uses the MIT license.";
    aboutPrompt.information(this, "About", aboutText);
    aboutPrompt.show();
}

void NarrativeDirector::on_actionSimplify_triggered()
{
    if(paragraphs.length() == 0) return;
    changeParagraphLbl(prgNum);
}

QString NarrativeDirector::getRecordingPath() {
    QString recordingFileDirName = QFileInfo(narrativeFile.fileName()).fileName();
    recordingFileDirName.chop(4);

    QString recordingPath = QStandardPaths::writableLocation(QStandardPaths::MusicLocation)
            + "/" + recordingFileDirName;

    return recordingPath;
}

uint NarrativeDirector::getNumPrgs() {
    uint numPrgs = 1;
    uint numSents = 1;
    narrativeInput.seek(0);

    while(!narrativeInput.atEnd()) {
        QString currentChar = narrativeInput.read(1);
        if(!isEndOfSentence(currentChar.front())) continue;

        if(++numSents % 4 == 0) numPrgs++;
        getToStartOfNextSentence();
    }

    narrativeInput.seek(0);
    filePos = narrativeInput.pos();

    return numPrgs;
}

void NarrativeDirector::getToStartOfNextSentence() {
    while(!narrativeInput.atEnd()) {
        QChar currentLetter = narrativeInput.read(1).front();

        if(isEndOfQuote(currentLetter)) continue;
        if(!isEndOfSentence(currentLetter)) break;
    }
}
