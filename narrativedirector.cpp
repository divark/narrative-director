#include "narrativedirector.h"
#include "ui_narrativedirector.h"

#include <QInputDialog>

NarrativeDirector::NarrativeDirector(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::NarrativeDirector) {
    ui->setupUi(this);

    audioRecorder = new QAudioRecorder(this);
    audioPlayer = new QMediaPlayer(this);
    preferences = new Preferences(this, audioRecorder);

    recordingPosition = QTime(0, 0, 0, 0);
    recordingDuration = QTime(0, 0, 0, 0);

    connect(audioRecorder, &QAudioRecorder::stateChanged, this,
            &NarrativeDirector::onARStateChanged);
    connect(audioRecorder, &QAudioRecorder::durationChanged, this,
            &NarrativeDirector::updateAEnd);

    connect(audioPlayer, &QMediaPlayer::stateChanged, this,
            &NarrativeDirector::onMPStateChanged);
    connect(audioPlayer, &QMediaPlayer::mediaStatusChanged, this,
            &NarrativeDirector::onMPMediaStatusChanged);
    connect(audioPlayer, &QMediaPlayer::positionChanged, this,
            &NarrativeDirector::updateAProgress);

    connect(audioRecorder,
            QOverload<QMediaRecorder::Error>::of(&QAudioRecorder::error), this,
            &NarrativeDirector::displayErrorMessage);
}

NarrativeDirector::~NarrativeDirector() {
    if (narrativeFile.isOpen())
        narrativeFile.close();

    delete audioRecorder;
    delete audioPlayer;
    delete ui;
    delete preferences;
}

//==============
// Slot functions
//==============

// File Context Menus
void NarrativeDirector::on_actionOpen_triggered() {
    if (hasChanged) {
        saveToProjectFile();
    }

    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Text File"),
                                                    QDir::currentPath(),
                                                    tr("Text files (*.txt)"));

    if (fileName.isNull())
        return;
    if (narrativeFile.isOpen())
        narrativeFile.close();

    narrativeFile.setFileName(fileName);
    if (!narrativeFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QString fileNameNoExt = QFileInfo(narrativeFile).fileName();
    fileNameNoExt = fileNameNoExt.left(fileNameNoExt.lastIndexOf("."));

    QFileInfo checkProjectFile(getRecordingPath() + "/" + fileNameNoExt +
                               ".ndp");
    if (checkProjectFile.exists() && checkProjectFile.isFile()) {
        cleanPrgs();

        loadFromProjectFile(checkProjectFile.filePath());
        updatePlayerInfo();

        ui->recordBtn->setEnabled(true);
        return;
    }

    prgNum = 0;
    filePos = 0;

    narrativeInput.flush();
    narrativeInput.reset();
    narrativeInput.setDevice(&narrativeFile);
    narrativeInput.setCodec("UTF-8");

    cleanPrgs();
    prgNumTotal = getNumPrgs();
    updatePlayerInfo();

    ui->recordBtn->setEnabled(true);
}

void NarrativeDirector::on_actionSave_triggered() {
    saveToProjectFile();
    hasChanged = false;
}

void NarrativeDirector::on_actionExport_Parts_File_triggered() {
    if (paragraphs.length() == 0) {
        showErrorMsg("There are no parts files to record.");
        return;
    }

    QString recordingPath = getRecordingPath();

    if (!QDir(recordingPath).exists() || !QFileInfo(recordingPath).isDir()) {
        showErrorMsg("Recording directory not present.");
        return;
    }

    QFile partsFile(recordingPath + "/" + "parts-list.txt");
    if (!partsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        showErrorMsg("File error for parts-list.txt creation");
        return;
    }

    QTextStream fileOutput(&partsFile);
    for (int i = 0; i < paragraphs.length(); i++) {
        QString recordingFileName =
            "part" + QString::number(i) + audioExtension;

        fileOutput << "file " << recordingPath << "/" << recordingFileName
                   << '\n'
                   << flush;
    }

    partsFile.close();

    QMessageBox::information(this, "Success",
                             "parts-list.txt created successfully.");
}

// Format context menus
void NarrativeDirector::on_actionSimplify_triggered() {
    if (paragraphs.length() == 0)
        return;
    changeParagraphLbl(prgNum);
}

// Edit context menus
void NarrativeDirector::on_actionPreferences_triggered() {
    preferences->show();
}

// About context menus
void NarrativeDirector::on_actionAbout_Narrative_Director_triggered() {
    const QString aboutText =
        "Narrative Director is a Qt program written to assist in audio "
        "recording."
        " It guides the narrator by recording in parts by paragraphs."
        " This program uses the MIT license.";

    QMessageBox::information(this, "About", aboutText);
}

// States
// Audio Recorder
void NarrativeDirector::onARStateChanged(QAudioRecorder::State state) {
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

void NarrativeDirector::updateAProgress(int duration) {
    if (duration < 1000)
        return;

    QTime newTime(0, 0, 0, 0);
    recordingPosition = newTime.addMSecs(duration);

    if (recordingPosition > recordingDuration)
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

// Media Player
void NarrativeDirector::onMPStateChanged(QMediaPlayer::State state) {
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

void NarrativeDirector::onMPMediaStatusChanged(
    QMediaPlayer::MediaStatus mediaStatus) {
    switch (mediaStatus) {
    case QMediaPlayer::LoadedMedia:
        recordingDuration =
            QTime(0, 0, 0, 0)
                .addMSecs(static_cast<int>(audioPlayer->duration()));

        ui->playBtn->setEnabled(true);
        ui->stopBtn->setEnabled(true);
        updateTimeLbl();
        break;
    case QMediaPlayer::InvalidMedia:
    case QMediaPlayer::UnknownMediaStatus:
    case QMediaPlayer::NoMedia:
        if (audioRecorder->state() == QAudioRecorder::RecordingState)
            return;
        recordingDuration = QTime(0, 0, 0, 0);

        ui->playBtn->setEnabled(false);
        ui->stopBtn->setEnabled(false);
        updateTimeLbl();
        break;
    default:
        break;
    }
}

// Buttons
void NarrativeDirector::on_recordBtn_clicked() {
#ifdef _WIN32
    auto filePath = recordingLocation.toLocalFile().toStdString().c_str();
    auto fileAttributes = GetFileAttributesA(filePath);

    if (fileAttributes != INVALID_FILE_ATTRIBUTES) {
        DeleteFileA(filePath);
        audioPlayer->setMedia(nullptr);
    }
#else
    if (QFileInfo::exists(recordingLocation.path())) {
        QFile recordingFile(recordingLocation.toLocalFile());

        recordingFile.remove();
    }
#endif
    updateRecordingLocation();

    audioRecorder->setOutputLocation(recordingLocation);
    audioRecorder->record();
}

void NarrativeDirector::on_playBtn_clicked() {
    // Audio player checks
    if (audioPlayer->state() == QMediaPlayer::PausedState ||
        audioPlayer->state() == QMediaPlayer::StoppedState) {
        audioPlayer->play();
    } else if (audioPlayer->state() == QMediaPlayer::PlayingState) {
        audioPlayer->pause();
    }

    // Audio recording check
    if (audioRecorder->state() == QAudioRecorder::RecordingState) {
        audioRecorder->pause();
    }
}

void NarrativeDirector::on_stopBtn_clicked() {
    if (audioRecorder->state() == QAudioRecorder::RecordingState) {
        audioRecorder->stop();

        auto outputLocation = audioRecorder->outputLocation();
        audioPlayer->setMedia(outputLocation);
        audioExtension = outputLocation.fileName().right(4);
        return;
    }

    // I must be playing audio otherwise, so
    audioPlayer->stop();
    recordingPosition = QTime(0, 0, 0, 0);
    updateTimeLbl();
}

void NarrativeDirector::on_backBtn_clicked() {
    if (prgNum - 1 < 0)
        return;

    recordingPosition = QTime(0, 0, 0, 0);
    updateTimeLbl();

    try {
        prgNum--;
        updatePlayerInfo();
    } catch (std::string &myError) {
        prgNum++;
        qDebug() << QString::fromStdString(myError);
    }
}

void NarrativeDirector::on_nextBtn_clicked() {
    if (paragraphs.length() == 0)
        return;

    recordingPosition = QTime(0, 0, 0, 0);
    updateTimeLbl();

    try {
        prgNum++;
        updatePlayerInfo();
    } catch (std::string &myError) {
        prgNum--;
        qDebug() << QString::fromStdString(myError);
    }
}

// Error Reporting
void NarrativeDirector::showErrorMsg(const QString &errorMsg) {
    QMessageBox::critical(this, "Error", errorMsg);
}

//========================
// Various helper functions
//========================
void NarrativeDirector::cleanPrgs() {
    paragraphs.clear();
    paragraphs.squeeze();
}

void NarrativeDirector::updatePlayerInfo() {
    changeParagraphLbl(prgNum);
    updateRecordingLocation();
    updatePlayerLocation();
}

void NarrativeDirector::displayErrorMessage() {
    showErrorMsg(audioRecorder->errorString());
}

void NarrativeDirector::updateTimeLbl() {
    ui->timeLbl->setText(recordingPosition.toString() + '/' +
                         recordingDuration.toString());
}

void NarrativeDirector::changeParagraphLbl(int prgIndex) {
    std::stringstream prgStream;
    QString paragraph = getParagraph(prgIndex);
    if (ui->actionSimplify->isChecked())
        paragraph = paragraph.simplified();

    prgStream << "Paragraph " << prgIndex + 1 << "/" << prgNumTotal;
    ui->prgText->setPlainText(paragraph);
    ui->prgLbl->setText(QString::fromStdString(prgStream.str()));
}

QString NarrativeDirector::getParagraphFromFile(qint64 location) {
    QString myParagraph = "";

    for (int i = 0; i < 4; i++) {
        QString sentence = getSentenceFromFile(location);

        if (sentence.isEmpty())
            break;
        myParagraph += sentence;
    }
    filePos = location;

    return myParagraph.trimmed();
}

QString NarrativeDirector::getSentenceFromFile(qint64 &location) {
    QString sentence = "";

    narrativeInput.seek(location);
    while (!narrativeInput.atEnd()) {
        QChar currentLetter = narrativeInput.read(1).front();

        sentence.append(currentLetter);

        if (isEndOfSentence(currentLetter)) {
            appendUntilNextSentence(sentence);
            break;
        }
    }
    location = narrativeInput.pos();

    return sentence;
}

bool NarrativeDirector::isEndOfSentence(const QChar &letter) {
    return letter == "!" || letter == "?" || letter == ".";
}

bool NarrativeDirector::isEndOfQuote(const QChar &letter) {
    return letter == "\"" or letter == "'" or letter == "”" or letter == "`";
}

void NarrativeDirector::appendUntilNextSentence(QString &sentence) {
    while (!narrativeInput.atEnd()) {
        QChar currentLetter = narrativeInput.read(1).front();

        if (isEndOfQuote(currentLetter) || !isEndOfSentence(currentLetter)) {
            sentence.append(currentLetter);
            break;
        }

        sentence.append(currentLetter);
    }
}

QString NarrativeDirector::getParagraph(int paragraphNum) {
    std::pair<qint64, QString> prgEntry;

    if (paragraphNum < paragraphs.length()) {
        prgEntry = paragraphs[paragraphNum];

        if (!prgEntry.second.isNull())
            return prgEntry.second;

        return getParagraphFromFile(prgEntry.first);
    }

    if (narrativeInput.atEnd())
        throw std::string("File at end.");

    prgEntry.first = filePos;
    QString paragraph = getParagraphFromFile(filePos);
    prgEntry.second = paragraph;

    paragraphs.push_back(prgEntry);

    return paragraph.trimmed();
}

void NarrativeDirector::saveToProjectFile() {
    QFile outputProjFile(getRecordingPath() + "/" + getNonExtensionFileName() +
                         ".ndp");
    if (!outputProjFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream fileOutput(&outputProjFile);
    fileOutput << prgNumTotal << '\n' << flush;
    for (auto &prgPair : paragraphs)
        fileOutput << prgPair.first << ",";
    fileOutput << '\n' << flush;
    fileOutput << prgNum << '\n' << flush;
    fileOutput << narrativeFile.fileName() << '\n' << flush;
    fileOutput << audioExtension << '\n' << flush;

    outputProjFile.close();
}

void NarrativeDirector::loadFromProjectFile(const QString &filePath) {
    QFile openedProjectFile(filePath);
    if (!openedProjectFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream prjInput(&openedProjectFile);

    // Number of paragraphs total.
    prgNumTotal = prjInput.readLine().toUInt();
    // paragraphs.reserve(prjInput.readLine().toInt());

    // Known paragraph locations
    QString prgStarts = prjInput.readLine();
    std::istringstream prgStream(prgStarts.toStdString());
    std::string foundPrgLoc;
    while (std::getline(prgStream, foundPrgLoc, ',')) {
        std::pair<int, QString> prgEntry;
        prgEntry.first = QString::fromStdString(foundPrgLoc).toInt();

        paragraphs.push_back(prgEntry);
    }

    // Paragraph user was last on.
    prgNum = prjInput.readLine().toInt();

    // Text file being read for narration.
    narrativeFile.setFileName(prjInput.readLine());
    if (!narrativeFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    this->narrativeInput.setDevice(&narrativeFile);
    this->narrativeInput.setCodec("UTF-8");

    // Audio extension for parts
    audioExtension = prjInput.readLine();

    // Last, but not least, the project file.
    currentProjectFile = filePath;

    openedProjectFile.close();
}

void NarrativeDirector::closeEvent(QCloseEvent *event) {
    if (hasChanged) {
        saveToProjectFile();
    }

    event->accept();
}

void NarrativeDirector::updateRecordingLocation() {
    const QString recordingPath = getRecordingPath();

    if (!QDir(recordingPath).exists())
        QDir().mkdir(recordingPath);

    const QString recordingFileName =
        "part" + QString::number(prgNum) + audioExtension;

    recordingLocation =
        QUrl::fromUserInput(recordingFileName, recordingPath,
                            QUrl::UserInputResolutionOption::AssumeLocalFile);
}

void NarrativeDirector::updatePlayerLocation() {
#ifdef _WIN32
    auto recordingPath = recordingLocation.toLocalFile();
#else
    auto recordingPath = recordingLocation.path();
#endif

    if (QFileInfo::exists(recordingPath))
        audioPlayer->setMedia(recordingLocation);
    else
        audioPlayer->setMedia(nullptr);
}

QString NarrativeDirector::getNonExtensionFileName() {
    QString recordingFileDirName =
        QFileInfo(narrativeFile.fileName()).fileName();

    return recordingFileDirName.left(recordingFileDirName.lastIndexOf("."));
}

QString NarrativeDirector::getRecordingPath() {
    QString recordingFileDirName = getNonExtensionFileName();

    QString recordingPath =
        QStandardPaths::writableLocation(QStandardPaths::MusicLocation) + "/" +
        recordingFileDirName;

    return recordingPath;
}

uint NarrativeDirector::getNumPrgs() {
    uint numPrgs = 1;
    uint numSents = 1;
    narrativeInput.seek(0);
    bool seenEndOfSentence = false;

    while (!narrativeInput.atEnd()) {
        QChar currentChar = narrativeInput.read(1).front();
        if (!isEndOfSentence(currentChar)) {
            seenEndOfSentence = false;
            continue;
        }

        if (isEndOfSentence(currentChar) && seenEndOfSentence) {
            continue;
        }

        if (++numSents % 4 == 0)
            numPrgs++;
        seenEndOfSentence = true;
    }

    narrativeInput.seek(0);
    filePos = narrativeInput.pos();

    return numPrgs;
}

void NarrativeDirector::on_actionGo_To_triggered() {
    if (paragraphs.length() == 0)
        return;
    bool isOkay = false;
    int paragraphNum = QInputDialog::getInt(this, tr("Goto Paragraph"),
                                            tr("Paragraph Number:"), 1, 1,
                                            paragraphs.length(), 1, &isOkay);

    if (!isOkay)
        return;

    int oldPrgNum = prgNum;
    try {
        prgNum = paragraphNum - 1;
        updatePlayerInfo();
    } catch (std::string &myError) {
        prgNum = oldPrgNum;
        qDebug() << QString::fromStdString(myError);
    }
}
