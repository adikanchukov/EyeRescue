#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDesktopWidget>
#include <QCloseEvent>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QTimer>

static QString const LOCK_TIME_SETTING = "lock_time";
static QString const REMIND_TIME_SETTING = "remind_time";
static quint8 const LOCK_TIME_DEFAULT = 20;
static quint8 const REMIND_TIME_DEFAULT = 1;
static quint16 const MILLISECONDS_PER_MIN = 60000;

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow{parent},
    ui{new Ui::MainWindow}, systemTray{new QSystemTrayIcon{this}}, activateTimer{new QTimer{this}},
    remindTimer{new QTimer{this}}, activateTime{0}, remindTime{0}
{
    ui->setupUi(this);

    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));

    readSettings();

    connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), SLOT(buttonBoxClicked(QAbstractButton*)));
    connect(ui->lockScreenSpinBox, SIGNAL(valueChanged(int)), SLOT(changeRemindBeforeMaxValue(int)));

    connect(activateTimer, SIGNAL(timeout()), SLOT(activate()));
    connect(remindTimer, SIGNAL(timeout()), SLOT(remind()));

    setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    initSystemTrayIcon();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::activate()
{
    stopTimers();

    if (lockScreen() == QProcess::NormalExit)
    {
        if (QMessageBox::question(this, qApp->applicationName(), "Ready to continue?") == QMessageBox::Yes)
            resetTimers();
    }
    else
    {
        QMessageBox::critical(this, qApp->applicationName(),
                              "Your lock screen is not available."
                              "\nPlease <a href = 'mailto:dikanchukov@mail.ru'>contact</a> developer.",
                              QMessageBox::Ok);
        qApp->quit();
    }
}

void MainWindow::remind()
{
    systemTray->showMessage(qApp->applicationName(), QString::number(remindTime) + " min. left");
}

void MainWindow::buttonBoxClicked(QAbstractButton* button)
{
    switch(ui->buttonBox->standardButton(button))
    {
    case QDialogButtonBox::RestoreDefaults:
    {
        readSettings();
        break;
    }
    case QDialogButtonBox::Apply:
    {
        writeSettings();
        hide();
        resetTimers();
        break;
    }
    default:
        break;
    }
}

void MainWindow::systemTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason)
    {
        case QSystemTrayIcon::Trigger:
        {
            show();
            return;
        }
        default:
            break;
    }
}

void MainWindow::changeRemindBeforeMaxValue(int value)
{
    ui->remindSpinBox->setMaximum(--value);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    hide();
    event->ignore();
}

void MainWindow::readSettings()
{
    QSettings const settings;

    activateTime = settings.value(LOCK_TIME_SETTING, LOCK_TIME_DEFAULT).toUInt();
    remindTime = settings.value(REMIND_TIME_SETTING, REMIND_TIME_DEFAULT).toUInt();

    ui->lockScreenSpinBox->setValue(activateTime);
    ui->remindSpinBox->setValue(remindTime);
}

void MainWindow::writeSettings()
{
    QSettings settings;

    activateTime = ui->lockScreenSpinBox->value();
    remindTime = ui->remindSpinBox->value();

    settings.setValue(LOCK_TIME_SETTING, activateTime);
    settings.setValue(REMIND_TIME_SETTING, remindTime);
}

void MainWindow::initSystemTrayIcon()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable() || !QSystemTrayIcon::supportsMessages())
    {
        QMessageBox::critical(this, qApp->applicationName(),
                              "Your system tray is not supported."
                              "\nPlease <a href = 'mailto:dikanchukov@mail.ru'>contact</a> developer.",
                              QMessageBox::Ok);
        qApp->quit();
    }


    QMenu* const trayMenu {new QMenu{this}};
    trayMenu->addAction("&Reset", this, SLOT(resetTimers()));
    trayMenu->addAction("Sto&p", this, SLOT(stopTimers()));
    trayMenu->addAction("&Settings", this, SLOT(show()));
    trayMenu->addSeparator();
    trayMenu->addAction("E&xit", qApp, SLOT(quit()));

    setActiveStatusIcon(false);

    connect(systemTray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            SLOT(systemTrayActivated(QSystemTrayIcon::ActivationReason)));

    systemTray->setContextMenu(trayMenu);
    systemTray->show();
}

int MainWindow::lockScreen() const
{
    QStringList const lockScreenCommands
    {
        "gnome-screensaver-command --lock",
        "xscreensaver-command --lock",
        "qdbus org.freedesktop.ScreenSaver /ScreenSaver Lock",
        "qdbus org.gnome.ScreenSaver /ScreenSaver Lock",
        "xlock"
    };

    foreach (QString const & command, lockScreenCommands)
        if (QProcess::execute(command) == QProcess::NormalExit)
            return QProcess::NormalExit;

    return QProcess::CrashExit;
}

void MainWindow::setActiveStatusIcon(bool active)
{
    if (active)
    {
        setWindowIcon(QIcon{":/icons/logo"});
        systemTray->setIcon(QIcon{":/icons/logo"});
    }
    else
    {
        setWindowIcon(QIcon{":/icons/logo_inactive"});
        systemTray->setIcon(QIcon{":/icons/logo_inactive"});
    }
}

void MainWindow::resetTimers()
{
    activateTimer->start(activateTime * MILLISECONDS_PER_MIN);
    remindTimer->start((activateTime - remindTime) * MILLISECONDS_PER_MIN);

    setActiveStatusIcon(true);
}

void MainWindow::stopTimers()
{
    activateTimer->stop();
    remindTimer->stop();

    setActiveStatusIcon(false);
}
