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
#include "preferences.h"

namespace Ui {
class NarrativeDirector;
}

class NarrativeDirector : public QMainWindow
{
    Q_OBJECT

public:
    explicit NarrativeDirector(QWidget *parent = nullptr);

    void changeParagraphLbl(int);
    void updateTimeLbl();
    void setPartsDir(QString);
    QString getParagraphFromFile(qint64);
    QString getParagraph(int);
    QString getSentenceFromFile(qint64&);
    void getToStartOfNextSentence(QString&, qint64&);
    bool isEndOfSentence(QChar);
    void saveToProjectFile(QString);
    void loadFromProjectFile(QString);

    ~NarrativeDirector() override;

private slots:
    void on_recordBtn_clicked();

    void on_playBtn_clicked();

    void on_backBtn_clicked();

    void on_nextBtn_clicked();

    void on_actionNew_triggered();

    void on_actionOpen_triggered();

    void on_stopBtn_clicked();

    void onARStateChanged(QAudioRecorder::State);
    void onMPStateChanged(QMediaPlayer::State);
    //void displayErrorMessage();

    void on_actionSave_As_triggered();

    void on_actionSave_triggered();

    void on_actionPreferences_triggered();
    void updateProgress(int pos);

private:
    Ui::NarrativeDirector *ui;
    Preferences *preferences;
    QAudioRecorder* audioRecorder = nullptr;
    QMediaPlayer* audioPlayer = nullptr;
    QTime currentTime;
    QTime audioFileDuration;

    QVector<std::pair<int, QString>> paragraphs;
    int prgNum = 0;

    QDir partsLocation;
    QFile currentFile;
    QTextStream fileInput;
    QDateTime fileLastModifed;
    qint64 filePos = 0;
    QString currentProjectFile;
    QString recordFile = "part0";
    QString audioExtension;
    bool hasChanged;

    void closeEvent(QCloseEvent *event) override;
    bool promptIfNotSaved();
    void changeRecordFile(int);
};

#endif // NARRATIVEDIRECTOR_H
