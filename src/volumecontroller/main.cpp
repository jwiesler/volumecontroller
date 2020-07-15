#include "runguard.h"
#include "volumecontroller/ui/volumecontroller.h"

#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <QPalette>
#include <QFile>
#include <QMutex>

static QFile logFile("VolumeController.log");
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

void test() {
	const std::array<int, 6> ints = {1, 2, 3, 4, 5, 6};

	auto arr = ints;
	const std::array<int, 3> remove = {1, 2, 5};
	const auto it = RemoveIndices(arr.begin(), arr.end(), remove.begin(), remove.end());
}

int main(int argc, char *argv[])
{
	test();
	RunGuard guard("volumecontroller");
	if(!guard.tryToRun())
		return 0;

	QByteArray envVar = qgetenv("QTDIR");       //  check if the app is ran in Qt Creator

//	if (envVar.isEmpty())
		logFile.open(QIODevice::Append | QIODevice::Text);

	qSetMessagePattern("%{time MM-dd hh:mm:ss:zzz} %{type} %{threadid} %{function}: %{message}");
	defaultMessageHandler = qInstallMessageHandler(messageHandler);

	qInfo() << "Starting VolumeController";
	if(logFile.isOpen()) {
		qDebug() << "Logging to file and console";
	} else {
		qDebug() << "Logging to console";
	}

	QApplication a(argc, argv);
	VolumeController w;
//	auto palette = w.palette();
//	palette.setColor(QPalette::Window, QColor::fromRgb(55, 55, 55));
//	palette.setColor(QPalette::WindowText, QColor(Qt::white));
//	palette.setColor(QPalette::ButtonText, QColor(214, 215, 215));

//	palette.setColor(QPalette::Mid, QColor(125, 126, 130));
//	palette.setColor(QPalette::Dark, QColor(125, 126, 130));
//	palette.setColor(QPalette::Light, QColor(125, 126, 130));
//	palette.setColor(QPalette::Base, QColor(125, 126, 130));
//	palette.setColor(QPalette::AlternateBase, QColor(125, 126, 130));
//	palette.setColor(QPalette::Button, QColor(125, 126, 130));
//	palette.setColor(QPalette::Link, QColor(125, 126, 130));
//	palette.setColor(QPalette::NoRole, QColor(125, 126, 130));
//	palette.setColor(QPalette::Midlight, QColor(125, 126, 130));
//	palette.setColor(QPalette::Shadow, QColor(125, 126, 130));

//	palette.setBrush(QPalette::Mid, QColor(125, 126, 130));
//	palette.setBrush(QPalette::Dark, QColor(125, 126, 130));
//	palette.setBrush(QPalette::Light, QColor(125, 126, 130));
//	palette.setBrush(QPalette::Base, QColor(125, 126, 130));
//	palette.setBrush(QPalette::AlternateBase, QColor(125, 126, 130));
//	palette.setBrush(QPalette::Button, QColor(125, 126, 130));
//	palette.setBrush(QPalette::Link, QColor(125, 126, 130));
//	palette.setBrush(QPalette::NoRole, QColor(125, 126, 130));
//	palette.setBrush(QPalette::Midlight, QColor(125, 126, 130));
//	palette.setBrush(QPalette::Shadow, QColor(125, 126, 130));

//	w.setPalette(palette);

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
