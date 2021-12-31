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

#include "loadsave.h"

#include <QByteArray>
#include <QDataStream>
#include <QFile>
#include <QSaveFile>
#include <QString>
#include <QTextStream>

#include "document.h"
#include "hamming.h"
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
				document->setPageNumberFromString(inLine.mid(3,3));
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
					// Import M/29 whole-magazine packets as X/28 per-page packets
					if (lineNumber == 29) {
						if ((document->pageNumber() & 0xff) != 0xff)
							qDebug("M/29/%d packet found, but page number is not xFF!", designationCode);
						lineNumber = 28;
					}
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

void importT42(QFile *inFile, TeletextDocument *document)
{
	unsigned char inLine[42];
	int readMagazineNumber, readPacketNumber;
	int foundMagazineNumber = -1;
	int foundPageNumber = -1;
	bool firstPacket0Found = false;
	bool pageBodyPacketsFound = false;

	for (;;) {
		if (inFile->read((char *)inLine, 42) != 42)
			// Reached end of .t42 file, or less than 42 bytes left
			break;

		// Magazine and packet numbers
		inLine[0] = hamming_8_4_decode[inLine[0]];
		inLine[1] = hamming_8_4_decode[inLine[1]];
		if (inLine[0] == 0xff || inLine[1] == 0xff)
			// Error decoding magazine or packet number
			continue;
		readMagazineNumber = inLine[0] & 0x07;
		readPacketNumber = (inLine[0] >> 3) | (inLine[1] << 1);

		if (readPacketNumber == 0) {
			// Hamming decode page number, subcodes and control bits
			for (int i=2; i<10; i++)
				inLine[i] = hamming_8_4_decode[inLine[i]];
			// See if the page number is valid
			if (inLine[2] == 0xff || inLine[3] == 0xff)
				// Error decoding page number
				continue;

			const int readPageNumber = (inLine[3] << 4) | inLine[2];

			if (readPageNumber == 0xff)
				// Time filling header
				continue;

			// A second or subsequent X/0 has been found
			if (firstPacket0Found) {
				if (readMagazineNumber != foundMagazineNumber)
					// Packet from different magazine broadcast in parallel mode
					continue;
				if ((readPageNumber == foundPageNumber) && pageBodyPacketsFound)
					// X/0 with same page number found after page body packets loaded - assume end of page
					break;
				if (readPageNumber != foundPageNumber) {
					// More than one page in .t42 file - end of current page reached
					qDebug("More than one page in .t42 file");
					break;
				}
				// Could get here if X/0 with same page number was found with no body packets inbetween
				continue;
			} else {
				// First X/0 found
				foundMagazineNumber = readMagazineNumber;
				foundPageNumber = readPageNumber;
				firstPacket0Found = true;

				if (foundMagazineNumber == 0)
					document->setPageNumber(0x800 | foundPageNumber);
				else
					document->setPageNumber((foundMagazineNumber << 8) | foundPageNumber);

				document->subPage(0)->setControlBit(PageBase::C4ErasePage, inLine[5] & 0x08);
				document->subPage(0)->setControlBit(PageBase::C5Newsflash, inLine[7] & 0x04);
				document->subPage(0)->setControlBit(PageBase::C6Subtitle, inLine[7] & 0x08);
				for (int i=0; i<4; i++)
					document->subPage(0)->setControlBit(PageBase::C7SuppressHeader+i, inLine[8] & (1 << i));
				document->subPage(0)->setControlBit(PageBase::C11SerialMagazine, inLine[9] & 0x01);
				document->subPage(0)->setControlBit(PageBase::C12NOS, inLine[9] & 0x08);
				document->subPage(0)->setControlBit(PageBase::C13NOS, inLine[9] & 0x04);
				document->subPage(0)->setControlBit(PageBase::C14NOS, inLine[9] & 0x02);

				continue;
			}
		}

		// No X/0 has been found yet, keep looking for one
		if (!firstPacket0Found)
			continue;

		// Disregard whole-magazine packets
		if (readPacketNumber > 28)
			continue;

		// We get here when a page-body packet belonging to the found X/0 header was found
		pageBodyPacketsFound = true;

		// At the moment this only loads a Level One Page properly
		// because it assumes X/1 to X/25 is odd partity
		if (readPacketNumber < 25) {
			for (int i=2; i<42; i++)
				// TODO - obey odd parity?
				inLine[i] &= 0x7f;
			document->subPage(0)->setPacket(readPacketNumber, QByteArray((const char *)&inLine[2], 40));
			continue;
		}

		// X/26, X/27 or X/28
		int readDesignationCode = hamming_8_4_decode[inLine[2]];

		if (readDesignationCode == 0xff)
			// Error decoding designation code
			continue;

		if (readPacketNumber == 27 && readDesignationCode < 4) {
			// X/27/0 to X/27/3 for Editorial Linking
			// Decode Hamming 8/4 on each of the six links, checking for errors on the way
			for (int i=0; i<6; i++) {
				bool decodingError = false;
				const int b = 3 + i*6; // First byte of this link

				for (int j=0; j<6; j++) {
					inLine[b+j] = hamming_8_4_decode[inLine[b+j]];
					if (inLine[b+j] == 0xff) {
						decodingError = true;
						break;
					}
				}

				if (decodingError) {
					// Error found in at least one byte of the link
					// Neutralise the whole link to same magazine, page FF, subcode 3F7F
					qDebug("X/27/%d link %d decoding error", readDesignationCode, i);
					inLine[b]   = 0xf;
					inLine[b+1] = 0xf;
					inLine[b+2] = 0xf;
					inLine[b+3] = 0x7;
					inLine[b+4] = 0xf;
					inLine[b+5] = 0x3;
				}
			}
			document->subPage(0)->setPacket(readPacketNumber, readDesignationCode, QByteArray((const char *)&inLine[2], 40));

			continue;
		}

		// X/26, or X/27/4 to X/27/15, or X/28
		// Decode Hamming 24/18
		for (int i=0; i<13; i++) {
			const int b = 3 + i*3; // First byte of triplet

			const int p0 = inLine[b];
			const int p1 = inLine[b+1];
			const int p2 = inLine[b+2];

			unsigned int D1_D4;
			unsigned int D5_D11;
			unsigned int D12_D18;
			unsigned int ABCDEF;
			int32_t d;

			D1_D4 = hamming_24_18_decode_d1_d4[p0 >> 2];
			D5_D11 = p1 & 0x7f;
			D12_D18 = p2 & 0x7f;

			d = D1_D4 | (D5_D11 << 4) | (D12_D18 << 11);

			ABCDEF = (hamming_24_18_parities[0][p0] ^ hamming_24_18_parities[1][p1]  ^ hamming_24_18_parities[2][p2]);

			d ^= (int)hamming_24_18_decode_correct[ABCDEF];

			if ((d & 0x80000000) == 0x80000000) {
				// Error decoding Hamming 24/18
				qDebug("X/%d/%d triplet %d decoding error", readPacketNumber, readDesignationCode, i);
				if (readPacketNumber == 26) {
					// Enhancements packet, set to "dummy" Address 41, Mode 0x1e, Data 0
					inLine[b]   = 41;
					inLine[b+1] = 0x1e;
					inLine[b+2] = 0;
				} else {
					// Zero out whole decoded triplet, bound to make things go wrong...
					inLine[b]   = 0x00;
					inLine[b+1] = 0x00;
					inLine[b+2] = 0x00;
				}
			} else {
				inLine[b]   = d & 0x0003f;
				inLine[b+1] = (d & 0x00fc0) >> 6;
				inLine[b+2] = d >> 12;
			}
		}
		document->subPage(0)->setPacket(readPacketNumber, readDesignationCode, QByteArray((const char *)&inLine[2], 40));
	}

	if (!firstPacket0Found)
		qDebug("No X/0 found");
	else if (!pageBodyPacketsFound)
		qDebug("X/0 found, but no page body packets were found");
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
		outStream << QString("PN,%1%2").arg(document.pageNumber(), 3, 16, QChar('0')).arg(subPageNumber & 0xff, 2, 10, QChar('0'));
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
			outStream << Qt::endl;
#else
			outStream << endl;
#endif

		// Subpage
		// Magazine Organisation Table and Magazine Inventory Page don't have subpages
		if (document.pageFunction() != TeletextDocument::PFMOT && document.pageFunction() != TeletextDocument::PFMIP) {
			outStream << QString("SC,%1").arg(subPageNumber, 4, 10, QChar('0'));
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

		// X/27 then X/28 always come first
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

void exportM29File(QSaveFile &file, const TeletextDocument &document)
{
	const PageBase &subPage = *document.currentSubPage();
	QTextStream outStream(&file);

	auto writeM29Packet=[&](int designationCode)
	{
		if (subPage.packetExists(28, designationCode)) {
			QByteArray outLine = subPage.packet(28, designationCode);

			outStream << QString("OL,29,");
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

	// Force page number to xFF
	outStream << QString("PN,%1ff00").arg(document.pageNumber() >> 8, 1, 16);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	outStream << Qt::endl;
#else
	outStream << endl;
#endif

	outStream << "PS,8000";
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	outStream << Qt::endl;
#else
	outStream << endl;
#endif

	writeM29Packet(0);
	writeM29Packet(1);
	writeM29Packet(4);
}

void exportT42File(QSaveFile &file, const TeletextDocument &document)
{
	const PageBase &subPage = *document.currentSubPage();

	QDataStream outStream(&file);
	// Displayable row header we export as spaces, hence the (odd parity valid) 0x20 init value
	QByteArray outLine(42, 0x20);
	int magazineNumber = (document.pageNumber() & 0xf00) >> 8;

	auto write7bitPacket=[&](int packetNumber)
	{
		if (subPage.packetExists(packetNumber)) {
			outLine[0] = hamming_8_4_encode[magazineNumber | ((packetNumber & 0x01) << 3)];
			outLine[1] = hamming_8_4_encode[packetNumber >> 1];
			outLine.replace(2, 40, subPage.packet(packetNumber));

			// Odd parity encoding
			for (int c=0; c<outLine.size(); c++) {
				char p = outLine.at(c);

				// Recursively divide integer into two equal halves and take their XOR until only 1 bit is left
				p ^= p >> 4;
				p ^= p >> 2;
				p ^= p >> 1;
				// If last bit left is 0 then it started with an even number of bits, so do the odd parity
				if (!(p & 1))
					outLine[c] = outLine.at(c) | 0x80;
			}
			outStream.writeRawData(outLine.constData(), 42);
		}
	};

	auto writeHamming8_4Packet=[&](int packetNumber, int designationCode=0)
	{
		if (subPage.packetExists(packetNumber, designationCode)) {
			outLine[0] = hamming_8_4_encode[magazineNumber | ((packetNumber & 0x01) << 3)];
			outLine[1] = hamming_8_4_encode[packetNumber >> 1];
			outLine.replace(2, 40, subPage.packet(packetNumber, designationCode));
			outLine[2] = hamming_8_4_encode[designationCode];

			for (int c=3; c<outLine.size(); c++)
				outLine[c] = hamming_8_4_encode[(int)outLine.at(c)];

			outStream.writeRawData(outLine.constData(), 42);
		}
	};

	auto writeHamming24_18Packet=[&](int packetNumber, int designationCode=0)
	{
		if (subPage.packetExists(packetNumber, designationCode)) {
			outLine[0] = hamming_8_4_encode[magazineNumber | ((packetNumber & 0x01) << 3)];
			outLine[1] = hamming_8_4_encode[packetNumber >> 1];
			outLine.replace(2, 40, subPage.packet(packetNumber, designationCode));
			outLine[2] = hamming_8_4_encode[designationCode];

			for (int c=3; c<outLine.size(); c+=3) {
				unsigned int D5_D11;
				unsigned int D12_D18;
				unsigned int P5, P6;
				unsigned int Byte_0;

				const unsigned int toEncode = outLine[c] | (outLine[c+1] << 6) | (outLine[c+2] << 12);

				Byte_0 = (hamming_24_18_forward[0][(toEncode >> 0) & 0xff] ^ hamming_24_18_forward[1][(toEncode >> 8) & 0xff] ^ hamming_24_18_forward_2[(toEncode >> 16) & 0x03]);
				outLine[c] = Byte_0;

				D5_D11 = (toEncode >> 4) & 0x7f;
				D12_D18 = (toEncode >> 11) & 0x7f;

				P5 = 0x80 & ~(hamming_24_18_parities[0][D12_D18] << 2);
				outLine[c+1] = D5_D11 | P5;

				P6 = 0x80 & ((hamming_24_18_parities[0][Byte_0] ^ hamming_24_18_parities[0][D5_D11]) << 2);
				outLine[c+2] = D12_D18 | P6;
			}

			outStream.writeRawData(outLine.constData(), 42);
		}
	};


	if (magazineNumber == 8)
		magazineNumber = 0;

	// Write X/0 separately as it features both Hamming 8/4 and 7-bit odd parity within
	outLine[0] = magazineNumber & 0x07;
	outLine[1] = 0; // Packet number 0
	outLine[2] = document.pageNumber() & 0x00f;
	outLine[3] = (document.pageNumber() & 0x0f0) >> 4;
	outLine[4] = 0; // Subcode S1 - always export as 0
	outLine[5] = subPage.controlBit(PageBase::C4ErasePage) << 3;
	outLine[6] = 0; // Subcode S3 - always export as 0
	outLine[7] = (subPage.controlBit(PageBase::C5Newsflash) << 2) | (subPage.controlBit(PageBase::C6Subtitle) << 3);
	outLine[8] = subPage.controlBit(PageBase::C7SuppressHeader) | (subPage.controlBit(PageBase::C8Update) << 2) | (subPage.controlBit(PageBase::C9InterruptedSequence) << 2) | (subPage.controlBit(PageBase::C10InhibitDisplay) << 3);
	outLine[9] = subPage.controlBit(PageBase::C11SerialMagazine) | (subPage.controlBit(PageBase::C14NOS) << 2) | (subPage.controlBit(PageBase::C13NOS) << 2) | (subPage.controlBit(PageBase::C12NOS) << 3);

	for (int i=0; i<10; i++)
		outLine[i] = hamming_8_4_encode[(int)outLine.at(i)];

	// If we allow text in the row header, we'd odd-parity encode it here

	outStream.writeRawData(outLine.constData(), 42);

	// After X/0, X/27 then X/28 always come next
	for (int i=0; i<4; i++)
		writeHamming8_4Packet(27, i);
	for (int i=4; i<16; i++)
		writeHamming24_18Packet(27, i);
	for (int i=0; i<16; i++)
		writeHamming24_18Packet(28, i);

	if (document.packetCoding() == TeletextDocument::Coding7bit) {
		// For 7 bit coding i.e. Level One Pages, X/26 are written before X/1 to X/25
		for (int i=0; i<16; i++)
			writeHamming24_18Packet(26, i);
		for (int i=1; i<=24; i++)
			write7bitPacket(i);
	} else {
		// For others (especially (G)POP pages) X/1 to X/25 are written before X/26
		if (document.packetCoding() == TeletextDocument::Coding18bit)
			for (int i=1; i<=25; i++)
				writeHamming24_18Packet(i);
		else if (document.packetCoding() == TeletextDocument::Coding4bit)
			for (int i=1; i<=25; i++)
				writeHamming8_4Packet(i);
		else
			qDebug("Exported broken file as page coding is not supported");
		for (int i=0; i<16; i++)
			writeHamming24_18Packet(26, i);
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
