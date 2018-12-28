#ifndef NARRATIVEDIRECTOR_H
#define NARRATIVEDIRECTOR_H

#include <QMainWindow>
#include <QVector>
#include <QAudioRecorder>
#include <QMediaPlayer>
#include <QFileDialog>
#include <sstream>
#include <QDebug>

namespace Ui {
class NarrativeDirector;
}

class NarrativeDirector : public QMainWindow
{
    Q_OBJECT

public:
    explicit NarrativeDirector(QWidget *parent = nullptr);

    void changeParagraphLbl(int);
    QString getParagraphFromFile(qint64&);
    QString getSentenceFromFile(qint64&);
    void getToStartOfNextSentence(QString&, qint64&);
    bool isEndOfSentence(QChar);

    ~NarrativeDirector();

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

private:
    Ui::NarrativeDirector *ui;
    QAudioRecorder* audioRecorder = nullptr;
    QMediaPlayer* audioPlayer = nullptr;
    QFile currentFile;
    QTextStream fileInput;

    QVector<QString> paragraphs;
    int prgIndex = 0;
    qint64 filePos = 0;
};

#endif // NARRATIVEDIRECTOR_H
