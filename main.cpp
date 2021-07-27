/*
 * Copyright (C) 2020, 2021 Gavin MacGregor
 *
 * This file is part of QTeletextMaker.
 *
 * QTeletextMaker is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QTeletextMaker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QTeletextMaker.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <QApplication>
#include <QCommandLineParser>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
	Q_INIT_RESOURCE(qteletextmaker);
	QApplication app(argc, argv);
	QApplication::setApplicationName("QTeletextMaker");
	QApplication::setApplicationDisplayName(QApplication::applicationName());
	QApplication::setOrganizationName("gkmac.co.uk");
	QApplication::setOrganizationDomain("gkmac.co.uk");
	QApplication::setApplicationVersion("0.4-alpha");
	QCommandLineParser parser;
	parser.setApplicationDescription(QApplication::applicationName());
	parser.addHelpOption();
	parser.addVersionOption();
	parser.addPositionalArgument("file", "The file(s) to open.");
	parser.process(app);

	MainWindow *mainWin = Q_NULLPTR;
	foreach (const QString &file, parser.positionalArguments()) {
		MainWindow *newWin = new MainWindow(file);
		newWin->tile(mainWin);
		newWin->show();
		mainWin = newWin;
	}

	if (!mainWin)
		mainWin = new MainWindow;
	mainWin->show();

	return app.exec();
}
