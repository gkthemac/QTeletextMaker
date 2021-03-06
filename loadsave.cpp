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

#include "loadsave.h"

#include <QByteArray>
#include <QFile>
#include <QSaveFile>
#include <QString>
#include <QTextStream>

#include "document.h"
#include "levelonepage.h"
#include "pagebase.h"

void loadTTI(QFile *inFile, TeletextDocument *document)
{
	QByteArray inLine;
	bool firstSubPageAlreadyFound = false;
	int cycleCommandsFound = 0;
	int mostRecentCycleValue = -1;
	LevelOnePage::CycleTypeEnum mostRecentCycleType;

	LevelOnePage* loadingPage = document->subPage(0);

	for (;;) {
		inLine = inFile->readLine(160).trimmed();
		if (inLine.isEmpty())
			break;
		if (inLine.startsWith("DE,"))
			document->setDescription(QString(inLine.remove(0, 3)));
		if (inLine.startsWith("PN,")) {
			// When second and subsequent PN commands are found, firstSubPageAlreadyFound==true at this point
			// This assumes that PN is the first command of a new subpage...
			if (firstSubPageAlreadyFound) {
				document->insertSubPage(document->numberOfSubPages(), false);
				loadingPage = document->subPage(document->numberOfSubPages()-1);
			} else {
				document->setPageNumber(inLine.mid(3,3));
				firstSubPageAlreadyFound = true;
			}
		}
/*		if (lineType == "SC,") {
			bool subPageNumberOk;
			int subPageNumberRead = inLine.mid(3, 4).toInt(&subPageNumberOk, 16);
			if ((!subPageNumberOk) || subPageNumberRead > 0x3f7f)
				subPageNumberRead = 0;
			loadingPage->setSubPageNumber(subPageNumberRead);
		}*/
		if (inLine.startsWith("PS,")) {
			bool pageStatusOk;
			int pageStatusRead = inLine.mid(3, 4).toInt(&pageStatusOk, 16);
			if (pageStatusOk) {
				loadingPage->setControlBit(PageBase::C4ErasePage, pageStatusRead & 0x4000);
				for (int i=PageBase::C5Newsflash, pageStatusBit=0x0001; i<=PageBase::C11SerialMagazine; i++, pageStatusBit<<=1)
					loadingPage->setControlBit(i, pageStatusRead & pageStatusBit);
				loadingPage->setDefaultNOS(((pageStatusRead & 0x0200) >> 9) | ((pageStatusRead & 0x0100) >> 7) | ((pageStatusRead & 0x0080) >> 5));
			}
		}
		if (inLine.startsWith("CT,") && (inLine.endsWith(",C") || inLine.endsWith(",T"))) {
			bool cycleValueOk;
			int cycleValueRead = inLine.mid(3, inLine.size()-5).toInt(&cycleValueOk);
			if (cycleValueOk) {
				cycleCommandsFound++;
				// House-keep CT command values, in case it's the only one within multiple subpages
				mostRecentCycleValue = cycleValueRead;
				loadingPage->setCycleValue(cycleValueRead);
				mostRecentCycleType = inLine.endsWith("C") ? LevelOnePage::CTcycles : LevelOnePage::CTseconds;
				loadingPage->setCycleType(mostRecentCycleType);
			}
		}
		if (inLine.startsWith("FL,")) {
			bool fastTextLinkOk;
			int fastTextLinkRead;
			QString flLine = QString(inLine.remove(0, 3));
			if (flLine.count(',') == 5)
				for (int i=0; i<6; i++) {
					fastTextLinkRead = flLine.section(',', i, i).toInt(&fastTextLinkOk, 16);
					if (fastTextLinkOk) {
						if (fastTextLinkRead == 0)
							fastTextLinkRead = 0x8ff;
						// Stored as page link with relative magazine number, convert from absolute page number that was read
						fastTextLinkRead ^= document->pageNumber() & 0x700;
						fastTextLinkRead &= 0x7ff; // Fixes magazine 8 to 0
						loadingPage->setFastTextLinkPageNumber(i, fastTextLinkRead);
					}
				}
		}
		if (inLine.startsWith("OL,")) {
			bool lineNumberOk;
			int lineNumber, secondCommaPosition;

			secondCommaPosition = inLine.indexOf(",", 3);
			if (secondCommaPosition != 4 && secondCommaPosition != 5)
				continue;

			lineNumber = inLine.mid(3, secondCommaPosition-3).toInt(&lineNumberOk, 10);
			if (lineNumberOk && lineNumber>=0 && lineNumber<=29) {
				inLine.remove(0, secondCommaPosition+1);
				if (lineNumber <= 25) {
					for (int c=0; c<40; c++) {
						// trimmed() helpfully removes CRLF line endings from the just-read line for us
						// But it also (un)helpfully removes spaces at the end of a 40 character line, so put them back
						if (c >= inLine.size())
							inLine.append(' ');
						if (inLine.at(c) & 0x80)
							inLine[c] = inLine.at(c) & 0x7f;
						else if (inLine.at(c) == 0x10)
							inLine[c] = 0x0d;
						else if (inLine.at(c) == 0x1b) {
							inLine.remove(c, 1);
							inLine[c] = inLine.at(c) & 0xbf;
						}
					}
					loadingPage->setPacket(lineNumber, inLine);
				} else {
					int designationCode = inLine.at(0) & 0x3f;
					if (inLine.size() < 40) {
						// OL is too short!
						if (lineNumber == 26) {
							// For a too-short enhancement triplets OL, first trim the line down to nearest whole triplet
							inLine.resize((inLine.size() / 3 * 3) + 1);
							// Then use "dummy" enhancement triplets to extend the line to the proper length
							for (int i=inLine.size(); i<40; i+=3)
								inLine.append("i^@"); // Address 41, Mode 0x1e, Data 0
						} else
							// For other triplet OLs and Hamming 8/4 OLs, just pad with zero data
							for (int i=inLine.size(); i<40; i++)
								inLine.append("@");
					}
					for (int i=1; i<=39; i++)
						inLine[i] = inLine.at(i) & 0x3f;
					loadingPage->setPacket(lineNumber, designationCode, inLine);
				}
			}
		}
	}
	// If there's more than one subpage but only one valid CT command was found, apply it to all subpages
	// I don't know if this is correct
	if (cycleCommandsFound == 1 && document->numberOfSubPages()>1)
		for (int i=0; i<document->numberOfSubPages(); i++) {
			document->subPage(i)->setCycleValue(mostRecentCycleValue);
			document->subPage(i)->setCycleType(mostRecentCycleType);
		}
}

// Used by saveTTI and HashString
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
		if (document.subPage(p)->packetExists(packetNumber)) {
			QByteArray outLine = document.subPage(p)->packet(packetNumber);

			outStream << QString("OL,%1,").arg(packetNumber);
			for (int c=0; c<outLine.size(); c++)
				if (outLine.at(c) < 0x20) {
					// TTI files are plain text, so put in escape followed by control code with bit 6 set
					outLine[c] = outLine.at(c) | 0x40;
					outLine.insert(c, 0x1b);
					c++;
				}
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
			outStream << outLine << Qt::endl;
#else
			outStream << outLine << endl;
#endif
		}
	};

	auto writeHammingPacket=[&](int packetNumber, int designationCode=0)
	{
		if (document.subPage(p)->packetExists(packetNumber, designationCode)) {
			QByteArray outLine = document.subPage(p)->packet(packetNumber, designationCode);

			outStream << QString("OL,%1,").arg(packetNumber);
			// TTI stores raw values with bit 6 set, doesn't do Hamming encoding
			outLine[0] = designationCode | 0x40;
			for (int c=1; c<outLine.size(); c++)
				outLine[c] = outLine.at(c) | 0x40;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
			outStream << outLine << Qt::endl;
#else
			outStream << outLine << endl;
#endif
		}
	};

	outStream.setCodec("ISO-8859-1");

	if (!document.description().isEmpty())
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
		outStream << "DE," << document.description() << Qt::endl;
#else
		outStream << "DE," << document.description() << endl;
#endif

	// TODO DS and SP commands

	// If there's just one subpage then we save it with a subcode of 0000
	// otherwise start with a subcode of 0001
	int subPageNumber = document.numberOfSubPages() > 1;

	for (p=0; p<document.numberOfSubPages(); p++) {

		// Page number
		outStream << QString("PN,%1%2").arg(document.pageNumber(), 3, 16, QChar('0')).arg(subPageNumber & 0xff, 2, 16, QChar('0'));
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
			outStream << Qt::endl;
#else
			outStream << endl;
#endif

		// Subpage
		// Magazine Organisation Table and Magazine Inventory Page don't have subpages
		if (document.pageFunction() != TeletextDocument::PFMOT && document.pageFunction() != TeletextDocument::PFMIP) {
			outStream << QString("SC,%1").arg(subPageNumber, 4, 16, QChar('0'));
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
			outStream << Qt::endl;
#else
			outStream << endl;
#endif
		}

		// Status bits
		outStream << QString("PS,%1").arg(0x8000 | controlBitsToPS(document.subPage(p)), 4, 16, QChar('0'));
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
		outStream << Qt::endl;
#else
		outStream << endl;
#endif

		// Cycle time
		if (document.pageFunction() == TeletextDocument::PFLevelOnePage)
			// Assume that only Level One Pages have configurable cycle times
			outStream << QString("CT,%1,%2").arg(document.subPage(p)->cycleValue()).arg(document.subPage(p)->cycleType()==LevelOnePage::CTcycles ? 'C' : 'T');
		else
			// X/28/0 specifies page function and coding but the PF command
			// should make it obvious to a human that this isn't a Level One Page
			outStream << QString("PF,%1,%2").arg(document.pageFunction()).arg(document.packetCoding());
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
		outStream << Qt::endl;
#else
		outStream << endl;
#endif

		// FastText links
		bool writeFLCommand = false;
		if (document.pageFunction() == TeletextDocument::PFLevelOnePage && document.subPage(p)->packetExists(27,0)) {
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
			outStream << Qt::endl;
#else
			outStream << endl;
#endif
		}

		subPageNumber++;
	}
}

QByteArray rowPacketAlways(PageBase *subPage, int packetNumber)
{
	if (subPage->packetExists(packetNumber))
		return subPage->packet(packetNumber);
	else
		return QByteArray(40, ' ');
}

QString exportHashStringPage(LevelOnePage *subPage)
{
	int hashDigits[1167]={0};
	int totalBits, charBit;
	const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	QString hashString;

	// TODO int editTFCharacterSet = 5;
	bool blackForeground = false;

	for (int r=0; r<25; r++) {
		QByteArray rowPacket = rowPacketAlways(subPage, r);
		for (int c=0; c<40; c++) {
			if (rowPacket.at(c) == 0x00 || rowPacket.at(c) == 0x10)
				blackForeground = true;
			for (int b=0; b<7; b++) {
				totalBits = (r * 40 + c) * 7 + b;
				charBit = ((rowPacket.at(c)) >> (6 - b)) & 0x01;
				hashDigits[totalBits / 6] |= charBit << (5 - (totalBits % 6));
			}
		}
	}

	hashString.append(QString("#%1:").arg(blackForeground ? 8 : 0, 1, 16));

	for (int i=0; i<1167; i++)
		hashString.append(base64[hashDigits[i]]);

	return hashString;
}

QString exportHashStringPackets(LevelOnePage *subPage)
{
	auto colourToHexString=[&](int whichCLUT)
	{
		QString resultHexString;

		for (int i=whichCLUT*8; i<whichCLUT*8+8; i++)
			resultHexString.append(QString("%1").arg(subPage->CLUT(i), 3, 16, QChar('0')));
		return resultHexString;
	};

	const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	QString result;

	if (subPage->packetExists(28,0) || subPage->packetExists(28,4)) {
		// X/28/0 and X/28/4 are duplicates apart from the CLUT definitions
		// Assemble the duplicate beginning and ending of both packets
		QString x28StringBegin, x28StringEnd;

		x28StringBegin.append(QString("00%1").arg((subPage->defaultCharSet() << 3) | subPage->defaultNOS(), 2, 16, QChar('0')).toUpper());
		x28StringBegin.append(QString("%1").arg((subPage->secondCharSet() << 3) | subPage->secondNOS(), 2, 16, QChar('0')).toUpper());
		x28StringBegin.append(QString("%1%2%3%4").arg(subPage->leftSidePanelDisplayed(), 1, 10).arg(subPage->rightSidePanelDisplayed(), 1, 10).arg(subPage->sidePanelStatusL25(), 1, 10).arg(subPage->sidePanelColumns(), 1, 16));

		x28StringEnd = QString("%1%2%3%4").arg(subPage->defaultScreenColour(), 2, 16, QChar('0')).arg(subPage->defaultRowColour(), 2, 16, QChar('0')).arg(subPage->blackBackgroundSubst(), 1, 10).arg(subPage->colourTableRemap(), 1, 10);

		if (subPage->packetExists(28,0))
			result.append(":X280=" + x28StringBegin + colourToHexString(2) + colourToHexString(3) + x28StringEnd);
		if (subPage->packetExists(28,4))
			result.append(":X284=" + x28StringBegin + colourToHexString(0) + colourToHexString(1) + x28StringEnd);
	}

	if (!subPage->enhancements()->isEmpty()) {
		result.append(":X26=");
		for (int i=0; i<subPage->enhancements()->size(); i++) {
			result.append(base64[subPage->enhancements()->at(i).data() >> 1]);
			result.append(base64[subPage->enhancements()->at(i).mode() | ((subPage->enhancements()->at(i).data() & 1) << 5)]);
			result.append(base64[subPage->enhancements()->at(i).address()]);
		}
	}

	result.append(QString(":PS=%1").arg(0x8000 | controlBitsToPS(subPage), 0, 16, QChar('0')));
	return result;
}
