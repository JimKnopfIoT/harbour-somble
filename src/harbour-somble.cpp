// harbour-somble — read & chart Omron blood-pressure measurements over BLE.
#include <QtQuick>
#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>
#include <sailfishapp.h>

#include "omron.h"

namespace {

// Launched from the icon the app runs under the mapplauncherd booster, and
// stderr does not reach the journal — which makes a Bluetooth problem
// impossible to diagnose on the device. Mirror every message into a file the
// Raw page can show instead.
QFile *g_logFile = nullptr;
QtMessageHandler g_previousHandler = nullptr;

void fileMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (g_logFile && g_logFile->isOpen()) {
        QTextStream out(g_logFile);
        out << QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"))
            << QLatin1Char(' ') << msg << QLatin1Char('\n');
        out.flush();
    }
    if (g_previousHandler)
        g_previousHandler(type, context, msg);
}

QString logPath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty())
        dir = QDir::homePath();
    QDir().mkpath(dir);
    return dir + QStringLiteral("/somble.log");
}

void installFileLogger()
{
    g_logFile = new QFile(logPath());
    // Truncate on every start: only the current run is ever interesting, and an
    // unbounded log on a phone is its own bug.
    if (g_logFile->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        g_previousHandler = qInstallMessageHandler(fileMessageHandler);
}

} // namespace

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    QScopedPointer<QQuickView> view(SailfishApp::createView());

    installFileLogger();
    qWarning() << "somble: started";

    Omron omron;
    view->rootContext()->setContextProperty(QStringLiteral("omron"), &omron);

    view->setSource(SailfishApp::pathTo(QStringLiteral("qml/harbour-somble.qml")));
    view->show();
    return app->exec();
}
