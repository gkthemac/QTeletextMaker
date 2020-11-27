/*
 * Copyright (C) 2020 Gavin MacGregor
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

#include "loadsave.h"

#include <QByteArray>
#include <QSaveFile>
#include <QString>

#include "document.h"
#include "pagebase.h"

// Used by TTI and hashstring
int controlBitsToPS(PageBase *subPage)
{
	// C4 Erase page is stored in bit 14
	int pageStatus = 0x8000 | (subPage->controlBit(PageBase::C4ErasePage) << 14); 
	// C5 to C11 stored in order from bits 1 to 6
	for (int i=PageBase::C5Newsflash; i<=PageBase::C11SerialMagazine; i++)
		pageStatus |= subPage->controlBit(i) << (i-1);
	// Apparently the TTI format stores the NOS bits backwards
	pageStatus |= subPage->controlBit(PageBase::C12NOS) << 9;
	pageStatus |= subPage->controlBit(PageBase::C13NOS) << 8;
	pageStatus |= subPage->controlBit(PageBase::C14NOS) << 7;
	return pageStatus;
}

void saveTTI(QSaveFile &file, const TeletextDocument &document)
{
	int p;
	QTextStream outStream(&file);

	auto write7bitPacket=[&](int packetNumber)
	{
		if (document.subPage(p)->packetNeeded(packetNumber)) {
			QByteArray outLine = document.subPage(p)->packet(packetNumber);

			outStream << QString("OL,%1,").arg(packetNumber);
			for (int c=0; c<outLine.size(); c++)
				if (outLine.at(c) < 0x20) {
					// TTI files are plain text, so put in escape followed by control code with bit 6 set
					outLine[c] = outLine.at(c) | 0x40;
					outLine.insert(c, 0x1b);
					c++;
				}
			outStream << outLine << Qt::endl;
		}
	};

	auto writeHammingPacket=[&](int packetNumber, int designationCode=0)
	{
		if (document.subPage(p)->packetNeeded(packetNumber, designationCode)) {
			QByteArray outLine = document.subPage(p)->packet(packetNumber, designationCode);

			outStream << QString("OL,%1,").arg(packetNumber);
			// TTI stores raw values with bit 7 set, doesn't do Hamming encoding
			outLine[0] = designationCode | 0x40;
			for (int c=1; c<outLine.size(); c++)
				outLine[c] = outLine.at(c) | 0x40;
			outStream << outLine << Qt::endl;
		}
	};

	outStream.setCodec("ISO-8859-1");

	if (!document.description().isEmpty())
		outStream << "DE," << document.description() << Qt::endl;

	// TODO DS and SP commands

	// If there's just one subpage then we save it with a subcode of 0000
	// otherwise start with a subcode of 0001
	int subPageNumber = document.numberOfSubPages() > 1;

	for (p=0; p<document.numberOfSubPages(); p++) {

		outStream << QString("PN,%1%2").arg(document.pageNumber(), 3, 16, QChar('0')).arg(subPageNumber & 0xff, 2, 16, QChar('0')) << Qt::endl;
		// Magazine Organisation Table and Magazine Inventory Page don't have subpages
		if (document.pageFunction() != TeletextDocument::PFMOT && document.pageFunction() != TeletextDocument::PFMIP)
			outStream << QString("SC,%1").arg(subPageNumber, 4, 16, QChar('0')) << Qt::endl;

		outStream << QString("PS,%1").arg(0x8000 | controlBitsToPS(document.subPage(p)), 4, 16, QChar('0')) << Qt::endl;

		if (document.pageFunction() == TeletextDocument::PFLevelOnePage)
			// Assume that only Level One Pages have configurable cycle times
			outStream << QString("CT,%1,%2").arg(document.subPage(p)->cycleValue()).arg(document.subPage(p)->cycleType()==LevelOnePage::CTcycles ? 'C' : 'T') << Qt::endl;
		else
			// X/28/0 specifies page function and coding but the PF command
			// should make it obvious to a human that this isn't a Level One Page
			outStream << QString("PF,%1,%2").arg(document.pageFunction()).arg(document.packetCoding()) << Qt::endl;

		bool writeFLCommand = false;
		if (document.pageFunction() == TeletextDocument::PFLevelOnePage && document.subPage(p)->packetNeeded(27,0)) {
			// Subpage has FastText links - if any link to a specific subpage, we need to write X/27/0 as raw
			// otherwise we write the links as a human-readable FL command later on
			writeFLCommand = true;
			// TODO uncomment this when we can edit FastText subpage links
			/*for (int i=0; i<6; i++)
				if (document.subPage(p)->fastTextLinkSubPageNumber(i) != 0x3f7f) {
					writeFLCommand = false;
					break;
				}*/
		}

		// X27 then X28 always come first
		for (int i=(writeFLCommand ? 1 : 0); i<16; i++)
			writeHammingPacket(27, i);
		for (int i=0; i<16; i++)
			writeHammingPacket(28, i);

		if (document.packetCoding() == TeletextDocument::Coding7bit) {
			// For 7 bit coding i.e. Level One Pages, X/26 are written before X/1 to X/25
			for (int i=0; i<16; i++)
				writeHammingPacket(26, i);
			for (int i=1; i<=24; i++)
				write7bitPacket(i);
		} else {
			// For others (especially (G)POP pages) X/1 to X/25 are written before X/26
			for (int i=1; i<=25; i++)
				writeHammingPacket(i);
			for (int i=0; i<16; i++)
				writeHammingPacket(26, i);
		}

		if (writeFLCommand) {
			outStream << "FL,";
			for (int i=0; i<6; i++) {
				// Stored as page link with relative magazine number, convert to absolute page number for display
				int absoluteLinkPageNumber = document.subPage(p)->fastTextLinkPageNumber(i) ^ (document.pageNumber() & 0x700);
				// Fix magazine 0 to 8
				if ((absoluteLinkPageNumber & 0x700) == 0x000)
					absoluteLinkPageNumber |= 0x800;

				outStream << QString("%1").arg(absoluteLinkPageNumber, 3, 16, QChar('0'));
				if (i<5)
					outStream << ',';
			}
			outStream << Qt::endl;
		}

		subPageNumber++;
	}
}
