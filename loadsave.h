/*
 * Copyright (C) 2020-2022 Gavin MacGregor
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

#ifndef LOADSAVE_H
#define LOADSAVE_H

#include <QByteArray>
#include <QFile>
#include <QSaveFile>
#include <QString>
#include <QTextStream>

#include "document.h"
#include "levelonepage.h"
#include "pagebase.h"

void loadTTI(QFile *, TeletextDocument *);
void importT42(QFile *, TeletextDocument *);

int controlBitsToPS(PageBase *);

void saveTTI(QSaveFile &, const TeletextDocument &);
void exportT42File(QSaveFile &, const TeletextDocument &);
void exportM29File(QSaveFile &, const TeletextDocument &);

QByteArray rowPacketAlways(PageBase *, int);

QString exportHashStringPage(LevelOnePage *);
QString exportHashStringPackets(LevelOnePage *subPage);

#endif
