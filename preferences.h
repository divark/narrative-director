#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QAudioRecorder>
#include <QDialog>
#include <QMultimedia>
#include <QSettings>

namespace Ui {
class Preferences;
}

class Preferences : public QDialog {
    Q_OBJECT

  public:
    explicit Preferences(QWidget *parent = nullptr,
                         QAudioRecorder *recorder = nullptr);
    ~Preferences();

  private slots:
    void on_buttonBox_accepted();

  private:
    Ui::Preferences *ui;
    QAudioRecorder *recorder;
    QSettings globalSettings;

    void populateFromGlobals();
};

#endif // PREFERENCES_H
