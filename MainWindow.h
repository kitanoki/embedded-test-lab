#pragma once

#include <QMainWindow>
#include <QStringList>

#include <functional>

class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProcess;
class QPushButton;
class QSpinBox;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    enum class RunMode {
        None,
        Encode,
        Decode
    };

    void setupUi();
    void appendLog(const QString &line);
    void startEncode();
    void startDecode();
    void stopTask();
    void saveLogToFile();
    void pollStatus();
    void startRemoteTask(RunMode mode);
    QString buildEncodeGstCommand() const;
    QString buildDecodeGstCommand() const;
    void runSshOneShot(const QString &remoteCommand, const std::function<void(int, const QString &, const QString &)> &onDone);
    void startTailLog(const QString &remoteLogFile);
    void stopTailLog();
    bool autoSaveAndClearLogView();
    QString sshBinary() const;
    QString sshTarget() const;
    QStringList sshConnectionArgs() const;
    bool isPlinkBinary() const;
    QString sshProgramForStart(QStringList &args) const;
    static QString shQuote(const QString &text);
    static QString sanitizeCaseName(const QString &raw);

    QLineEdit *m_hostEdit = nullptr;
    QLineEdit *m_portEdit = nullptr;
    QLineEdit *m_userEdit = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    QLineEdit *m_hostKeyEdit = nullptr;
    QLineEdit *m_caseEdit = nullptr;
    QLineEdit *m_sshPathEdit = nullptr;

    QComboBox *m_channelBox = nullptr;
    QComboBox *m_resolutionBox = nullptr;
    QComboBox *m_codecBox = nullptr;
    QComboBox *m_rcBox = nullptr;

    QSpinBox *m_bitrateBox = nullptr;
    QSpinBox *m_gopBox = nullptr;

    QLabel *m_tempLabel = nullptr;
    QLabel *m_cpuLabel = nullptr;
    QLabel *m_memLabel = nullptr;
    QLabel *m_stateLabel = nullptr;

    QPlainTextEdit *m_logEdit = nullptr;

    QPushButton *m_startEncodeBtn = nullptr;
    QPushButton *m_startDecodeBtn = nullptr;
    QPushButton *m_stopBtn = nullptr;
    QPushButton *m_saveLogBtn = nullptr;

    QTimer *m_statusTimer = nullptr;
    QProcess *m_tailProcess = nullptr;

    QString m_runningPid;
    QString m_remoteLogFile;
    RunMode m_mode = RunMode::None;
    bool m_statusBusy = false;
};


