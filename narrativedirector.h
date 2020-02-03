#ifndef NARRATIVEDIRECTOR_H
#define NARRATIVEDIRECTOR_H

#include "preferences.h"
#include <QAudioRecorder>
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QMainWindow>
#include <QMediaPlayer>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTime>
#include <QVector>
#include <sstream>
#include <utility>

#ifdef _WIN32
#include <tchar.h>
#include <windows.h>
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
    QString getSentenceFromFile(qint64 &);

    void updatePlayerTimeLbl();
    void updateRecorderTimeLbl();
    void updateRecordingLocation();
    void updatePlayerLocation();

    void saveToProjectFile();
    void loadFromProjectFile(const QString &);

    ~NarrativeDirector() override;

  private slots:
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionExport_Parts_File_triggered();
    void on_actionPreferences_triggered();
    void on_actionSimplify_triggered();
    void on_actionAbout_Narrative_Director_triggered();

    void onARStateChanged(QAudioRecorder::State);
    void updateAProgress(int);
    void updateAEnd(int);

    void onMPStateChanged(QMediaPlayer::State);
    void onMPMediaStatusChanged(QMediaPlayer::MediaStatus);

    void on_recordBtn_clicked();
    void on_playBtn_clicked();
    void on_stopBtn_clicked();
    void on_backBtn_clicked();
    void on_nextBtn_clicked();

    void displayErrorMessage();

    void on_actionGo_To_triggered();

    void on_playbackSldr_sliderPressed();

    void on_playbackSldr_sliderMoved(int position);

private:
    Ui::NarrativeDirector *ui;
    Preferences *preferences;
    QAudioRecorder *audioRecorder = nullptr;
    QMediaPlayer *audioPlayer = nullptr;
    QUrl recordingLocation;

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

    bool isEndOfSentence(const QChar &);
    bool isEndOfQuote(const QChar &);
    void appendUntilNextSentence(QString &);

    void closeEvent(QCloseEvent *event) override;
    void showErrorMsg(const QString &);
};

#endif // NARRATIVEDIRECTOR_H
