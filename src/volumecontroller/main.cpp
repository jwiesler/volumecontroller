#include "runguard.h"
#include "volumecontroller/ui/customstyle.h"
#include "volumecontroller/ui/theme.h"
#include "volumecontroller/ui/volumecontroller.h"

#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <QPalette>
#include <QFile>
#include <QMutex>
#include <QTranslator>
#include <QStyleFactory>

const QString logFileName = "VolumeController.log";

static QFile logFile(logFileName);
static QtMessageHandler defaultMessageHandler;

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
	static QMutex logMutex;
	QMutexLocker lock(&logMutex);

	defaultMessageHandler(type, context, msg);

	if(logFile.isOpen()) {
		logFile.write(qFormatLogMessage(type, context, msg).toUtf8() + '\n');
		logFile.flush();
	}
}

#ifdef ROTATE_LOG_FILE
void rotateOldFile() {
	if(logFile.exists()) {
		const QString oldLogFileName = logFileName + ".0";
		QFile::remove(oldLogFileName);
		QFile::rename(logFileName, oldLogFileName);
	}
}
#endif

void enableLogToFile() {
	rotateOldFile();
	logFile.open(QIODevice::Append | QIODevice::Text);
}

int main(int argc, char *argv[])
{
	RunGuard guard("volumecontroller");
	if(!guard.tryToRun())
		return 0;

	QByteArray envVar = qgetenv("QTDIR");       //  check if the app is ran in Qt Creator

//	if (envVar.isEmpty())
		enableLogToFile();

	qSetMessagePattern("%{time MM-dd hh:mm:ss:zzz} %{type} %{threadid} %{function}: %{message}");
	defaultMessageHandler = qInstallMessageHandler(messageHandler);

	qInfo() << "Starting VolumeController";
	if(logFile.isOpen()) {
		qDebug() << "Logging to file and console";
	} else {
		qDebug() << "Logging to console";
	}

	QApplication a(argc, argv);

	auto *ptr = QStyleFactory::create("windowsvista");
	Q_ASSERT(ptr);
	CustomStyle style(ptr);
	QApplication::setStyle(&style);

	VolumeController w(nullptr, style);
	w.setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
	w.setAttribute(Qt::WA_TranslucentBackground);
	w.setAttribute(Qt::WA_QuitOnClose);

	QObject::connect(&a, &QApplication::applicationStateChanged, [&](const Qt::ApplicationState state) {
		if(state == Qt::ApplicationState::ApplicationInactive)
			w.onApplicationInactive(a.activeWindow());
	});
//	w.fadeIn();

	return a.exec();
}
