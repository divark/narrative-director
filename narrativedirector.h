#ifndef NARRATIVEDIRECTOR_H
#define NARRATIVEDIRECTOR_H

#include <QMainWindow>
#include <QVector>
#include <QAudioRecorder>
#include <QMediaPlayer>
#include <QFileDialog>
#include <sstream>
#include <QDebug>
#include <utility>
#include <QFileInfo>
#include <QDateTime>
#include <QTime>
#include <QMessageBox>
#include <QStandardPaths>
#include "preferences.h"

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#endif

namespace Ui {
class NarrativeDirector;
}

class NarrativeDirector : public QMainWindow {
    Q_OBJECT

public:
    explicit NarrativeDirector(QWidget *parent = nullptr);

    void changeParagraphLbl(int);
    uint getNumPrgs();
    QString getParagraphFromFile(qint64);
    QString getParagraph(int);
    QString getSentenceFromFile(qint64&);

    void updateTimeLbl();
    void updateRecordingLocation();
    void updatePlayerLocation();

    void saveToProjectFile();
    void loadFromProjectFile(const QString&);

    ~NarrativeDirector() override;

private slots:
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionExport_Parts_File_triggered();
    void on_actionPreferences_triggered();
    void on_actionSimplify_triggered();
    void on_actionAbout_Narrative_Director_triggered();

    void onARStateChanged(QAudioRecorder::State);
    void updateAProgress(int pos);
    void updateAEnd(int pos);

    void onMPStateChanged(QMediaPlayer::State);
    void onMPMediaStatusChanged(QMediaPlayer::MediaStatus);

    void on_recordBtn_clicked();
    void on_playBtn_clicked();
    void on_stopBtn_clicked();
    void on_backBtn_clicked();
    void on_nextBtn_clicked();

    void displayErrorMessage();

private:
    Ui::NarrativeDirector *ui;
    Preferences *preferences;
    QAudioRecorder* audioRecorder = nullptr;
    QMediaPlayer* audioPlayer = nullptr;
    QUrl recordingLocation;
    QTime recordingPosition;
    QTime recordingDuration;

    QVector<std::pair<int, QString>> paragraphs;
    int prgNum = 0;
    uint prgNumTotal = 0;

    QFile narrativeFile;
    QTextStream narrativeInput;
    qint64 filePos = 0;
    QString currentProjectFile;
    bool hasChanged = false;
    QString audioExtension;

    void updatePlayerInfo();
    void cleanPrgs();

    QString getRecordingPath();
    QString getNonExtensionFileName();
    bool promptIfNotSaved();

    bool isEndOfSentence(const QString&);
    bool isEndOfQuote(const QString&);
    void appendUntilNextSentence(QString&, qint64&);

    void closeEvent(QCloseEvent *event) override;
    void showErrorMsg(const QString&);
};

#endif // NARRATIVEDIRECTOR_H
