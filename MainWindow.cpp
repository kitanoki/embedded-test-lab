#include "MainWindow.h"

#include <QComboBox>
#include <QDate>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();

    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &MainWindow::pollStatus);
    m_statusTimer->start(1000);

    connect(m_startEncodeBtn, &QPushButton::clicked, this, &MainWindow::startEncode);
    connect(m_startDecodeBtn, &QPushButton::clicked, this, &MainWindow::startDecode);
    connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::stopTask);
    connect(m_saveLogBtn, &QPushButton::clicked, this, &MainWindow::saveLogToFile);
}

MainWindow::~MainWindow()
{
    stopTailLog();
}

void MainWindow::setupUi()
{
    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);

    auto *sshGroup = new QGroupBox("SSH", central);
    auto *sshLayout = new QFormLayout(sshGroup);
    m_hostEdit = new QLineEdit("192.168.1.100", sshGroup);
    m_portEdit = new QLineEdit("22", sshGroup);
    m_userEdit = new QLineEdit("root", sshGroup);
    m_passwordEdit = new QLineEdit("CNDlive@0918", sshGroup);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_hostKeyEdit = new QLineEdit("SHA256:4quUgeL9Cv8yJ5tSE3z52DpyLNRkLlZt4ZXblssQTBM", sshGroup);
    m_caseEdit = new QLineEdit("EncodeCase", sshGroup);
    m_sshPathEdit = new QLineEdit("ssh", sshGroup);
    sshLayout->addRow("Host", m_hostEdit);
    sshLayout->addRow("Port", m_portEdit);
    sshLayout->addRow("User", m_userEdit);
    sshLayout->addRow("Password", m_passwordEdit);
    sshLayout->addRow("Host Key", m_hostKeyEdit);
    sshLayout->addRow("CaseName", m_caseEdit);
    sshLayout->addRow("SSH Binary", m_sshPathEdit);

    auto *paramGroup = new QGroupBox("Encode Parameters", central);
    auto *paramLayout = new QGridLayout(paramGroup);
    m_channelBox = new QComboBox(paramGroup);
    m_channelBox->addItems({"/dev/video11", "/dev/video12", "/dev/video13"});

    m_resolutionBox = new QComboBox(paramGroup);
    m_resolutionBox->addItems({"1920x1080", "3840x2160", "1280x720"});

    m_codecBox = new QComboBox(paramGroup);
    m_codecBox->addItems({"H264", "H265"});

    m_rcBox = new QComboBox(paramGroup);
    m_rcBox->addItems({"CBR", "VBR"});

    m_bitrateBox = new QSpinBox(paramGroup);
    m_bitrateBox->setRange(2, 40);
    m_bitrateBox->setValue(8);
    m_bitrateBox->setSuffix(" Mbps");

    m_gopBox = new QSpinBox(paramGroup);
    m_gopBox->setRange(2, 60);
    m_gopBox->setValue(30);

    paramLayout->addWidget(new QLabel("Channel", paramGroup), 0, 0);
    paramLayout->addWidget(m_channelBox, 0, 1);
    paramLayout->addWidget(new QLabel("Resolution", paramGroup), 0, 2);
    paramLayout->addWidget(m_resolutionBox, 0, 3);
    paramLayout->addWidget(new QLabel("Codec", paramGroup), 1, 0);
    paramLayout->addWidget(m_codecBox, 1, 1);
    paramLayout->addWidget(new QLabel("Rate Control", paramGroup), 1, 2);
    paramLayout->addWidget(m_rcBox, 1, 3);
    paramLayout->addWidget(new QLabel("Bitrate", paramGroup), 2, 0);
    paramLayout->addWidget(m_bitrateBox, 2, 1);
    paramLayout->addWidget(new QLabel("KeyFrame Interval", paramGroup), 2, 2);
    paramLayout->addWidget(m_gopBox, 2, 3);

    auto *buttonLayout = new QHBoxLayout();
    m_startEncodeBtn = new QPushButton("Start Encode", central);
    m_startDecodeBtn = new QPushButton("Start Decode", central);
    m_stopBtn = new QPushButton("Stop", central);
    m_saveLogBtn = new QPushButton("Save Logs", central);
    buttonLayout->addWidget(m_startEncodeBtn);
    buttonLayout->addWidget(m_startDecodeBtn);
    buttonLayout->addWidget(m_stopBtn);
    buttonLayout->addWidget(m_saveLogBtn);

    auto *statusGroup = new QGroupBox("Status", central);
    auto *statusLayout = new QHBoxLayout(statusGroup);
    m_tempLabel = new QLabel("Temp: --", statusGroup);
    m_cpuLabel = new QLabel("CPU: --", statusGroup);
    m_memLabel = new QLabel("MEM: --", statusGroup);
    m_stateLabel = new QLabel("State: Idle", statusGroup);
    statusLayout->addWidget(m_tempLabel);
    statusLayout->addWidget(m_cpuLabel);
    statusLayout->addWidget(m_memLabel);
    statusLayout->addWidget(m_stateLabel);

    auto *logGroup = new QGroupBox("Logs", central);
    auto *logLayout = new QVBoxLayout(logGroup);
    m_logEdit = new QPlainTextEdit(logGroup);
    m_logEdit->setReadOnly(true);
    logLayout->addWidget(m_logEdit);

    root->addWidget(sshGroup);
    root->addWidget(paramGroup);
    root->addLayout(buttonLayout);
    root->addWidget(statusGroup);
    root->addWidget(logGroup, 1);

    setCentralWidget(central);
    setWindowTitle("RK3576 Codec Test Console");
    resize(1100, 780);
}

void MainWindow::appendLog(const QString &line)
{
    m_logEdit->appendPlainText(line);
}

QString MainWindow::sshBinary() const
{
    const QString path = m_sshPathEdit->text().trimmed();
    return path.isEmpty() ? QStringLiteral("ssh") : path;
}

QString MainWindow::sshTarget() const
{
    return QStringLiteral("%1@%2").arg(m_userEdit->text().trimmed(), m_hostEdit->text().trimmed());
}

QStringList MainWindow::sshConnectionArgs() const
{
    QStringList args;

    if (isPlinkBinary()) {
        const QString port = m_portEdit->text().trimmed();
        if (!port.isEmpty()) {
            args << "-P" << port;
        }

        const QString password = m_passwordEdit->text();
        if (!password.isEmpty()) {
            args << "-pw" << password;
        }

        const QString hostKey = m_hostKeyEdit->text().trimmed();
        if (!hostKey.isEmpty()) {
            args << "-hostkey" << hostKey;
        }

        args << "-batch";
        return args;
    }

    const QString password = m_passwordEdit->text();
    const bool hasSshpass = !QStandardPaths::findExecutable("sshpass").isEmpty();

    const QString port = m_portEdit->text().trimmed();
    if (!port.isEmpty()) {
        args << "-p" << port;
    }

    args << "-o" << QString("BatchMode=%1").arg(password.isEmpty() || !hasSshpass ? "yes" : "no")
         << "-o" << "ConnectTimeout=8"
         << "-o" << "ServerAliveInterval=5"
         << "-o" << "ServerAliveCountMax=1"
         << "-o" << "StrictHostKeyChecking=accept-new";
    return args;
}

bool MainWindow::isPlinkBinary() const
{
    return sshBinary().contains("plink", Qt::CaseInsensitive);
}

QString MainWindow::sshProgramForStart(QStringList &args) const
{
    const QString password = m_passwordEdit->text();
    if (password.isEmpty()) {
        return sshBinary();
    }

    if (isPlinkBinary()) {
        return sshBinary();
    }

    const QString sshpass = QStandardPaths::findExecutable("sshpass");
    if (!sshpass.isEmpty()) {
        args.prepend(password);
        args.prepend("-p");
        args.prepend(sshBinary());
        return sshpass;
    }

    return sshBinary();
}

QString MainWindow::shQuote(const QString &text)
{
    QString out = text;
    out.replace("'", "'\"'\"'");
    return QString("'%1'").arg(out);
}

QString MainWindow::sanitizeCaseName(const QString &raw)
{
    QString caseName = raw.trimmed();
    caseName.replace(QRegularExpression("[^A-Za-z0-9_-]"), "_");
    if (caseName.isEmpty()) {
        caseName = "Case";
    }
    return caseName;
}

void MainWindow::runSshOneShot(const QString &remoteCommand, const std::function<void(int, const QString &, const QString &)> &onDone)
{
    auto *proc = new QProcess(this);

    connect(proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError err) {
        appendLog(QString("[Error] SSH process error: %1").arg(static_cast<int>(err)));
    });

    connect(proc, &QProcess::finished, this, [this, proc, onDone](int exitCode, QProcess::ExitStatus) {
        const QString out = QString::fromUtf8(proc->readAllStandardOutput());
        const QString err = QString::fromUtf8(proc->readAllStandardError());
        if (err.contains("Cannot confirm a host key in batch mode", Qt::CaseInsensitive)) {
            appendLog("[Hint] Plink host key is not trusted yet. Fill 'Host Key' field (for example SHA256 fingerprint) or import the key once via interactive plink/putty.");
        }
        onDone(exitCode, out, err);
        proc->deleteLater();
    });

    QStringList args = sshConnectionArgs();
    args << sshTarget() << remoteCommand;

    const QString password = m_passwordEdit->text();
    const bool usingPlink = isPlinkBinary();
    const bool usingSshpass = !password.isEmpty() && !QStandardPaths::findExecutable("sshpass").isEmpty() && !usingPlink;
    if (!password.isEmpty() && !usingPlink && !usingSshpass) {
        appendLog("[Warn] Password is set, but sshpass is not found. Current ssh command cannot input password interactively. Install sshpass, switch SSH Binary to plink, or use SSH key login.");
    }

    const QString program = sshProgramForStart(args);
    proc->start(program, args);
}

QString MainWindow::buildEncodeGstCommand() const
{
    const QString res = m_resolutionBox->currentText();
    const QStringList wh = res.split('x');
    const QString width = wh.value(0, "1920");
    const QString height = wh.value(1, "1080");

    const bool h265 = m_codecBox->currentText() == "H265";
    const QString enc = h265 ? "mpph265enc" : "mpph264enc";
    const QString parser = h265 ? "h265parse" : "h264parse";
    const QString rcMode = m_rcBox->currentText() == "VBR" ? "vbr" : "cbr";

    const int bps = m_bitrateBox->value() * 1000 * 1000;
    const int gop = m_gopBox->value();
    const QString dev = m_channelBox->currentText();

    return QStringLiteral("gst-launch-1.0 -e "
                          "v4l2src device=%1 ! "
                          "video/x-raw,width=%2,height=%3,framerate=30/1 ! "
                          "%4 bps=%5 gop=%6 rc-mode=%7 ! "
                          "%8 ! fakesink")
        .arg(dev, width, height, enc, QString::number(bps), QString::number(gop), rcMode, parser);
}

QString MainWindow::buildDecodeGstCommand() const
{
    return QStringLiteral("gst-launch-1.0 filesrc location=/usr/bin/DolbyVision_NASA_4K.mp4 ! decodebin ! autovideosink");
}

void MainWindow::startEncode()
{
    startRemoteTask(RunMode::Encode);
}

void MainWindow::startDecode()
{
    startRemoteTask(RunMode::Decode);
}

void MainWindow::startRemoteTask(RunMode mode)
{
    const QString caseName = sanitizeCaseName(m_caseEdit->text());
    const QString loopBody = QStringLiteral("while true; do %1; done").arg(mode == RunMode::Encode ? buildEncodeGstCommand() : buildDecodeGstCommand());

    const QString remote = QStringLiteral(
                               "mkdir -p /userdata/log; "
                               "pkill -f '/etc/loop.sh' >/dev/null 2>&1 || true; "
                               "killall -9 luajit >/dev/null 2>&1 || true; "
                               "log_file=\"/userdata/log/%1_$(date +%F).log\"; "
                               "nohup bash -c %2 > \"$log_file\" 2>&1 & "
                               "pid=$!; echo PID:$pid; echo LOG:$log_file;")
                               .arg(caseName, shQuote(loopBody));

    appendLog(QString("[Local] Starting remote %1 task...").arg(mode == RunMode::Encode ? "encode" : "decode"));

    runSshOneShot(remote, [this, mode](int exitCode, const QString &out, const QString &err) {
        if (exitCode != 0) {
            appendLog("[Error] SSH start command failed");
            if (!err.trimmed().isEmpty()) {
                appendLog(err.trimmed());
            }
            if (!out.trimmed().isEmpty()) {
                appendLog(out.trimmed());
            }
            if (err.trimmed().isEmpty() && out.trimmed().isEmpty()) {
                appendLog("[Hint] SSH returned no output. Check SSH Binary path, host/port reachability, and plink host key setting.");
            }
            m_stateLabel->setText("State: Start failed");
            return;
        }

        if (!err.trimmed().isEmpty()) {
            appendLog(err.trimmed());
        }

        appendLog(out.trimmed());

        const QStringList lines = out.split('\n', Qt::SkipEmptyParts);
        QString pid;
        QString logFile;
        for (const QString &line : lines) {
            if (line.startsWith("PID:")) {
                pid = line.mid(4).trimmed();
            } else if (line.startsWith("LOG:")) {
                logFile = line.mid(4).trimmed();
            }
        }

        if (pid.isEmpty() || logFile.isEmpty()) {
            appendLog("[Error] Did not parse PID/LOG from remote output.");
            m_stateLabel->setText("State: Parse failed");
            return;
        }

        m_runningPid = pid;
        m_remoteLogFile = logFile;
        m_mode = mode;
        m_stateLabel->setText(QString("State: Running (%1), PID=%2").arg(mode == RunMode::Encode ? "Encode" : "Decode", pid));

        startTailLog(logFile);
    });
}

void MainWindow::stopTask()
{
    if (m_runningPid.isEmpty()) {
        appendLog("[Local] No active PID.");
        m_stateLabel->setText("State: Idle");
        stopTailLog();
        m_mode = RunMode::None;
        return;
    }

    const QString remote = QStringLiteral("kill -9 %1 >/dev/null 2>&1 || true; echo STOPPED:%1;").arg(m_runningPid);

    runSshOneShot(remote, [this](int exitCode, const QString &out, const QString &err) {
        if (exitCode != 0) {
            appendLog("[Error] SSH stop command failed");
            if (!err.trimmed().isEmpty()) {
                appendLog(err.trimmed());
            }
            if (!out.trimmed().isEmpty()) {
                appendLog(out.trimmed());
            }
            return;
        }

        if (!err.trimmed().isEmpty()) {
            appendLog(err.trimmed());
        }
        appendLog(out.trimmed());

        stopTailLog();
        m_runningPid.clear();
        m_remoteLogFile.clear();
        m_mode = RunMode::None;
        m_stateLabel->setText("State: Idle");
    });
}

void MainWindow::startTailLog(const QString &remoteLogFile)
{
    stopTailLog();

    m_tailProcess = new QProcess(this);

    connect(m_tailProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        appendLog(QString::fromUtf8(m_tailProcess->readAllStandardOutput()).trimmed());
    });
    connect(m_tailProcess, &QProcess::readyReadStandardError, this, [this]() {
        appendLog(QString::fromUtf8(m_tailProcess->readAllStandardError()).trimmed());
    });
    connect(m_tailProcess, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
        appendLog(QString("[Local] tail exited: %1").arg(code));
    });

    QStringList args = sshConnectionArgs();
    args << sshTarget() << QString("tail -n 0 -F %1").arg(shQuote(remoteLogFile));

    const QString program = sshProgramForStart(args);
    m_tailProcess->start(program, args);
}

void MainWindow::stopTailLog()
{
    if (!m_tailProcess) {
        return;
    }

    m_tailProcess->kill();
    m_tailProcess->deleteLater();
    m_tailProcess = nullptr;
}

void MainWindow::pollStatus()
{
    if (m_statusBusy) {
        return;
    }

    m_statusBusy = true;

    const QString cmd =
        "temp_raw=$(cat /sys/class/thermal/thermal_zone0/temp 2>/dev/null || echo 0); "
        "temp=$(awk \"BEGIN { printf \\\"%.1f\\\", $temp_raw/1000 }\"); "
        "cpu=$(top -bn1 | awk '/Cpu\\(s\\)|^%Cpu/ {for(i=1;i<=NF;i++) if($i ~ /id,?/) {print 100-$(i-1); exit}}'); "
        "mem=$(free | awk '/Mem:/ {printf \"%.1f\", $3/$2*100}'); "
        "echo TEMP:$temp; echo CPU:$cpu; echo MEM:$mem;";

    runSshOneShot(cmd, [this](int, const QString &out, const QString &) {
        const QStringList lines = out.split('\n', Qt::SkipEmptyParts);

        for (const QString &line : lines) {
            if (line.startsWith("TEMP:")) {
                m_tempLabel->setText(QString("Temp: %1 C").arg(line.mid(5).trimmed()));
            } else if (line.startsWith("CPU:")) {
                m_cpuLabel->setText(QString("CPU: %1 %").arg(line.mid(4).trimmed()));
            } else if (line.startsWith("MEM:")) {
                m_memLabel->setText(QString("MEM: %1 %").arg(line.mid(4).trimmed()));
            }
        }

        m_statusBusy = false;
    });
}

void MainWindow::saveLogToFile()
{
    const QString defaultName = QString("rk3576_console_%1.log").arg(QDate::currentDate().toString("yyyy-MM-dd"));
    const QString filePath = QFileDialog::getSaveFileName(this, "Save Logs", defaultName, "Log Files (*.log);;All Files (*)");
    if (filePath.isEmpty()) {
        return;
    }

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        appendLog("[Error] Failed to save local log file.");
        return;
    }

    const QByteArray data = m_logEdit->toPlainText().toUtf8();
    f.write(data);
    f.close();

    appendLog(QString("[Local] Logs saved to %1").arg(filePath));
}

