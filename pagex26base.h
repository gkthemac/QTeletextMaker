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

#ifndef PAGEX26BASE_H
#define PAGEX26BASE_H

#include <QByteArray>
#include <QList>

#include "pagebase.h"
#include "x26triplets.h"

class PageX26Base : public PageBase //: public QObject
{
	//Q_OBJECT

public:
	QList<X26Triplet> *enhancements() { return &m_enhancements; };
	virtual int maxEnhancements() const =0;

protected:
	QByteArray packetFromEnhancementList(int) const;
	void setEnhancementListFromPacket(int, QByteArray);
	bool packetFromEnhancementListNeeded(int n) const { return ((m_enhancements.size()+12) / 13) > n; };

	QList<X26Triplet> m_enhancements;
	const X26Triplet m_paddingX26Triplet { 41, 0x1e, 0 };
};

#endif
