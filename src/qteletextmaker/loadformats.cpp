/*
 * Copyright (C) 2020-2025 Gavin MacGregor
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

#include "loadformats.h"

#include <QByteArray>
#include <QDataStream>
#include <QFile>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "hamming.h"
#include "levelonepage.h"
#include "pagebase.h"

bool LoadTTIFormat::load(QFile *inFile, QList<PageBase>& subPages, QVariantHash *metadata)
{
	m_warnings.clear();
	m_error.clear();

	QByteArray inLine;
	int pageNum = 0;
	int currentSubPageNum = 0;
	bool firstSubPageAlreadyFound = false;
	bool pageBodyPacketsFound = false;

//	subPages.clear();
	subPages.append(PageBase { } );

	PageBase* loadingPage = &subPages[0];

	for (;;) {
		inLine = inFile->readLine(160).trimmed();
		if (inLine.isEmpty())
			break;
		if (inLine.startsWith("DE,") && metadata != nullptr)
			metadata->insert("description", QString(inLine.remove(0, 3)));
		if (inLine.startsWith("PN,")) {
			if (!firstSubPageAlreadyFound) {
				// First PN command found, set the page number
				bool valueOk;

				if (int pageNumRead = inLine.mid(3, 3).toInt(&valueOk, 16); valueOk)
					if (pageNumRead >= 0x100 && pageNumRead <= 0x8ff) {
						// Keep page number: to check if page is xFF if we load M/29
						pageNum = pageNumRead;
						if (metadata != nullptr)
							metadata->insert("pageNumber", pageNum);
					}

				firstSubPageAlreadyFound = true;
			} else {
				// Subsequent PN command found; this assumes that PN is the first command of a new subpage
				currentSubPageNum++;
				subPages.append(PageBase { } );
				loadingPage = &subPages[subPages.size()-1];
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
			const int pageStatusRead = inLine.mid(3, 4).toInt(&pageStatusOk, 16);
			if (pageStatusOk) {
				loadingPage->setControlBit(PageBase::C4ErasePage, pageStatusRead & 0x4000);

				for (int i=PageBase::C5Newsflash, pageStatusBit=0x0001; i<=PageBase::C11SerialMagazine; i++, pageStatusBit<<=1)
					loadingPage->setControlBit(i, pageStatusRead & pageStatusBit);

				loadingPage->setControlBit(PageBase::C12NOS, pageStatusRead & 0x0200);
				loadingPage->setControlBit(PageBase::C13NOS, pageStatusRead & 0x0100);
				loadingPage->setControlBit(PageBase::C14NOS, pageStatusRead & 0x0080);
			}
		}
		if (inLine.startsWith("RE,")) {
			bool regionValueOk;
			const int regionValueRead = inLine.remove(0, 3).toInt(&regionValueOk);
			if (regionValueOk && metadata != nullptr && regionValueRead >= 0 && regionValueRead <= 15)
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
				metadata->insert(QString("region%1").arg(currentSubPageNum, 3, '0'), regionValueRead);
#else
				metadata->insert(QString("region%1").arg(currentSubPageNum, 3, QChar('0')), regionValueRead);
#endif
		}
		if (inLine.startsWith("CT,") && (inLine.endsWith(",C") || inLine.endsWith(",T"))) {
			bool cycleValueOk;
			const int cycleValueRead = inLine.mid(3, inLine.size()-5).toInt(&cycleValueOk);
			if (cycleValueOk && metadata != nullptr && cycleValueRead >= 1 && cycleValueRead <= 99) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
				metadata->insert(QString("cycleValue%1").arg(currentSubPageNum, 3, '0'), cycleValueRead);
				metadata->insert(QString("cycleType%1").arg(currentSubPageNum, 3, '0'), inLine.at(inLine.size()-1));
#else
				metadata->insert(QString("cycleValue%1").arg(currentSubPageNum, 3, QChar('0')), cycleValueRead);
				metadata->insert(QString("cycleType%1").arg(currentSubPageNum, 3, QChar('0')), inLine.at(inLine.size()-1));
#endif
			}
		}
		if (inLine.startsWith("FL,")) {
			const QString flLine = QString(inLine.remove(0, 3));
			if (flLine.count(',') == 5) {
				// Init packet to all 0xf's as page xFF:3F7F means no page is specified
				QByteArray fastTextPacket(40, 0xf);
				fastTextPacket[0] = 0x0;  // Designation code
				fastTextPacket[38] = 0x0; // CRC word
				fastTextPacket[39] = 0x0; // CRC word

				for (int i=0; i<6; i++) {
					bool fastTextLinkOk;
					int fastTextLinkRead = flLine.section(',', i, i).toInt(&fastTextLinkOk, 16);

					if (fastTextLinkOk) {
						if (fastTextLinkRead == 0)
							fastTextLinkRead = 0x8ff;
						else if (fastTextLinkRead >= 0x100 && fastTextLinkRead <= 0x8ff) {
							fastTextPacket[i*6+1] = fastTextLinkRead & 0x00f;
							fastTextPacket[i*6+2] = (fastTextLinkRead & 0x0f0) >> 4;
							fastTextPacket[i*6+4] = 0x7 | ((fastTextLinkRead & 0x100) >> 5);
							fastTextPacket[i*6+6] = 0x3 | ((fastTextLinkRead & 0x600) >> 7);

							loadingPage->setPacket(27, 0, fastTextPacket);
						}
					}
				}
				if (metadata != nullptr)
					metadata->insert(QString("fastextAbsolute"), true);
			}
		}
		if (inLine.startsWith("OL,")) {
			bool lineNumberOk;
			int lineNumber;

			const int secondCommaPosition = inLine.indexOf(',', 3);
			if (secondCommaPosition != 4 && secondCommaPosition != 5)
				continue;

			lineNumber = inLine.mid(3, secondCommaPosition-3).toInt(&lineNumberOk, 10);
			if (lineNumberOk && lineNumber >= 0 && lineNumber <= 29) {
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
					pageBodyPacketsFound = true;
					loadingPage->setPacket(lineNumber, inLine);
				} else if (inLine.at(0) >= 0x40 && inLine.at(0) <= 0x4f) {
					const int designationCode = inLine.at(0) & 0x3f;
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
						if ((pageNum & 0xff) != 0xff)
							m_warnings.append(QString("M/29/%1 packet found, but page number was not xFF.").arg(designationCode));
						lineNumber = 28;
					}
					pageBodyPacketsFound = true;
					loadingPage->setPacket(lineNumber, designationCode, inLine);
				}
			}
		}
	}

	if (!pageBodyPacketsFound) {
		m_error = "No OL lines found";
		return false;
	}

	return true;
}


bool LoadT42Format::readPacket()
{
	return m_inFile->read((char *)m_inLine, 42) == 42;
}

bool LoadT42Format::load(QFile *inFile, QList<PageBase>& subPages, QVariantHash *metadata)
{
	int readMagazineNumber, readPacketNumber;
	int foundMagazineNumber = -1;
	int foundPageNumber = -1;
	bool firstPacket0Found = false;
	bool pageBodyPacketsFound = false;

	m_inFile = inFile;

	m_warnings.clear();
	m_error.clear();
	m_reExportWarning = false;

//	subPages.clear();
	subPages.append(PageBase { });

	PageBase* loadingPage = &subPages[0];

	for (;;) {
		if (!readPacket())
			// Reached end of .t42 file, or less than 42 bytes left
			break;

		// Magazine and packet numbers
		m_inLine[0] = hamming_8_4_decode[m_inLine[0]];
		m_inLine[1] = hamming_8_4_decode[m_inLine[1]];
		if (m_inLine[0] == 0xff || m_inLine[1] == 0xff)
			// Error decoding magazine or packet number
			continue;
		readMagazineNumber = m_inLine[0] & 0x07;
		readPacketNumber = (m_inLine[0] >> 3) | (m_inLine[1] << 1);

		if (readPacketNumber == 0) {
			// Hamming decode page number, subcodes and control bits
			for (int i=2; i<10; i++)
				m_inLine[i] = hamming_8_4_decode[m_inLine[i]];
			// See if the page number is valid
			if (m_inLine[2] == 0xff || m_inLine[3] == 0xff)
				// Error decoding page number
				continue;

			const int readPageNumber = (m_inLine[3] << 4) | m_inLine[2];

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
					m_warnings.append("More than one page in .t42 file, only first full page loaded.");
					m_reExportWarning = true;
					break;
				}
				// Could get here if X/0 with same page number was found with no body packets inbetween
				continue;
			} else {
				// First X/0 found
				foundMagazineNumber = readMagazineNumber;
				foundPageNumber = readPageNumber;
				firstPacket0Found = true;

				if (metadata != nullptr) {
					if (foundMagazineNumber == 0)
						metadata->insert("pageNumber", 0x800 | foundPageNumber);
					else
						metadata->insert("pageNumber", (foundMagazineNumber << 8) | foundPageNumber);
				}

				loadingPage->setControlBit(PageBase::C4ErasePage, m_inLine[5] & 0x08);
				loadingPage->setControlBit(PageBase::C5Newsflash, m_inLine[7] & 0x04);
				loadingPage->setControlBit(PageBase::C6Subtitle, m_inLine[7] & 0x08);
				for (int i=0; i<4; i++)
					loadingPage->setControlBit(PageBase::C7SuppressHeader+i, m_inLine[8] & (1 << i));
				loadingPage->setControlBit(PageBase::C11SerialMagazine, m_inLine[9] & 0x01);
				loadingPage->setControlBit(PageBase::C12NOS, m_inLine[9] & 0x08);
				loadingPage->setControlBit(PageBase::C13NOS, m_inLine[9] & 0x04);
				loadingPage->setControlBit(PageBase::C14NOS, m_inLine[9] & 0x02);

				// See if there's text in the header row
				bool headerText = false;

				for (int i=10; i<42; i++)
					if (m_inLine[i] != 0x20) {
						// TODO - obey odd parity?
						m_inLine[i] &= 0x7f;
						headerText = true;
					}
				if (headerText) {
					// Clear the page address and control bits to spaces before putting the row in
					for (int i=0; i<10; i++)
						m_inLine[i] = 0x20;

					loadingPage->setPacket(0, QByteArray((const char *)&m_inLine[2], 40));
				}
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
				m_inLine[i] &= 0x7f;
			loadingPage->setPacket(readPacketNumber, QByteArray((const char *)&m_inLine[2], 40));
			continue;
		}

		// X/26, X/27 or X/28
		int readDesignationCode = hamming_8_4_decode[m_inLine[2]];

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
					m_inLine[b+j] = hamming_8_4_decode[m_inLine[b+j]];
					if (m_inLine[b+j] == 0xff) {
						decodingError = true;
						break;
					}
				}

				if (decodingError) {
					// Error found in at least one byte of the link
					// Neutralise the whole link to same magazine, page FF, subcode 3F7F
					qDebug("X/27/%d link %d decoding error", readDesignationCode, i);
					m_inLine[b]   = 0xf;
					m_inLine[b+1] = 0xf;
					m_inLine[b+2] = 0xf;
					m_inLine[b+3] = 0x7;
					m_inLine[b+4] = 0xf;
					m_inLine[b+5] = 0x3;
				}
			}
			loadingPage->setPacket(readPacketNumber, readDesignationCode, QByteArray((const char *)&m_inLine[2], 40));

			continue;
		}

		// X/26, or X/27/4 to X/27/15, or X/28
		// Decode Hamming 24/18
		for (int i=0; i<13; i++) {
			const int b = 3 + i*3; // First byte of triplet

			const int p0 = m_inLine[b];
			const int p1 = m_inLine[b+1];
			const int p2 = m_inLine[b+2];

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
					m_inLine[b]   = 41;
					m_inLine[b+1] = 0x1e;
					m_inLine[b+2] = 0;
				} else {
					// Zero out whole decoded triplet, bound to make things go wrong...
					m_inLine[b]   = 0x00;
					m_inLine[b+1] = 0x00;
					m_inLine[b+2] = 0x00;
				}
			} else {
				m_inLine[b]   = d & 0x0003f;
				m_inLine[b+1] = (d & 0x00fc0) >> 6;
				m_inLine[b+2] = d >> 12;
			}
		}
		loadingPage->setPacket(readPacketNumber, readDesignationCode, QByteArray((const char *)&m_inLine[2], 40));
	}

	if (!firstPacket0Found) {
		m_error = "No X/0 found.";
		return false;
	} else if (!pageBodyPacketsFound) {
		m_error = "X/0 found, but no page body packets were found.";
		return false;
	} else
		return true;
}


bool LoadHTTFormat::readPacket()
{
	unsigned char httLine[45];

	if (m_inFile->read((char *)httLine, 45) != 45)
		return false;

	if (httLine[0] != 0xaa || httLine[1] != 0xaa || httLine[2] != 0xe4)
		return false;

	for (int i=0; i<42; i++) {
		unsigned char b = httLine[i+3];
		b = (b & 0xf0) >> 4 | (b & 0x0f) << 4;
		b = (b & 0xcc) >> 2 | (b & 0x33) << 2;
		b = (b & 0xaa) >> 1 | (b & 0x55) << 1;
		m_inLine[i] = b;
	}

	return true;
}


bool LoadEP1Format::load(QFile *inFile, QList<PageBase>& subPages, QVariantHash *metadata)
{
	m_warnings.clear();
	m_error.clear();
	m_reExportWarning = false;

	unsigned char inLine[42];
	unsigned char numOfSubPages = 1;

//	subPages.clear();
	subPages.append(PageBase { } );

	PageBase* loadingPage = &subPages[0];

	for (;;) {
		// Read six bytes, will either be a header for a (sub)page
		// or a start header indicating multiple subpages are within
		if (inFile->read((char *)inLine, 6) != 6)
			return false;
		if (inLine[0] == 'J' || inLine[1] == 'W' || inLine[2] == 'C') {
			// Multiple subpages: get number of subpages then read
			// next six bytes that really will be the header of the first subpage
			numOfSubPages = inLine[3];
			if (inFile->read((char *)inLine, 6) != 6)
				return false;

			m_warnings.append("More than one page in EP1/EPX file, only first full page loaded.");
			m_reExportWarning = true;
		}

		// Check for header of a (sub)page
		if (inLine[0] != 0xfe || inLine[1] != 0x01)
			return false;

		// Deal with language code unique to EP1 - unknown values are mapped to English
		if (metadata != nullptr)
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
			metadata->insert(QString("region%1").arg(0, 3, '0'), m_languageCode.key(inLine[2], 0x09) >> 3);
#else
			metadata->insert(QString("region%1").arg(0, 3, QChar('0')), m_languageCode.key(inLine[2], 0x09) >> 3);
#endif

		const int nationalOption = m_languageCode.key(inLine[2], 0x09) & 0x7;

		loadingPage->setControlBit(PageBase::C12NOS, nationalOption & 0x1);
		loadingPage->setControlBit(PageBase::C13NOS, nationalOption & 0x2);
		loadingPage->setControlBit(PageBase::C14NOS, nationalOption & 0x4);

		// If fourth byte is 0xca then "X/26 enhancements header" follows
		// Otherwise Level 1 page data follows
		if (inLine[3] == 0xca) {
			// Read next four bytes that form the "X/26 enhancements header"
			if (inFile->read((char *)inLine, 4) != 4)
				return false;
			// Third and fourth bytes are little-endian length of enhancement data
			const int numOfX26Bytes = inLine[2] | (inLine[3] << 8);
			const int numOfX26Packets = (numOfX26Bytes + 39) / 40;

			QByteArray packet(40, 0x00);
			packet[0] = 0;

			for (int i=0; i<numOfX26Packets; i++) {
				bool terminatorFound = false;
				unsigned char terminatorTriplet[3];

				if (inFile->read((char *)inLine, 40) != 40)
					return false;

				// Assumes that X/26 packets are saved with ascending designation codes...
				for (int c=1; c<39; c+=3) {
					if (!terminatorFound) {
						// Shuffle triplet bits from 6 bit address, 5 bit mode, 7 bit data
						packet[c] = inLine[c];
						packet[c+1] = inLine[c+1] | ((inLine[c+2] & 1) << 5);
						packet[c+2] = inLine[c+2] >> 1;
						// Address of termination marker is 7f instead of 3f
						if (inLine[c+1] == 0x1f && inLine[c] == 0x7f) {
							packet[c] = 0x3f;

							if (inLine[c+2] & 0x01) {
								// If a termination marker was found, stop reading the packet
								// and repeat the marker ourselves to the end
								terminatorFound = true;
								terminatorTriplet[0] = packet[c+1];
								terminatorTriplet[1] = packet[c+2];
							}
						}
					} else {
						packet[c] = 0x3f;
						packet[c+1] = terminatorTriplet[0];
						packet[c+2] = terminatorTriplet[1];
					}
				}

				loadingPage->setPacket(26, i, packet);
			}
		}

		// Level 1 rows
		for (int r=0; r<24; r++) {
			if (inFile->read((char *)inLine, 40) != 40)
				return false;

			for (int c=0; c<40; c++)
				if (inLine[c] != 0x20) {
					loadingPage->setPacket(r, QByteArray((const char *)&inLine, 40));
					break;
				}
		}

		numOfSubPages--;

		// FIXME uncomment "if" statement when we're ready to save multi-page EPX files
		//if (numOfSubPages == 0)
		break;

		// There are more subpages coming up so skip over the 40 byte buffer and 2 byte terminator
		if (inFile->read((char *)inLine, 42) != 42)
			return false;

		subPages.append(PageBase { } );
		loadingPage = &subPages[subPages.size()-1];
	}
	return true;
}


int LoadFormats::s_instances = 0;

LoadFormats::LoadFormats()
{
	if (s_instances == 0) {
		s_fileFormat[0] = new LoadTTIFormat;
		s_fileFormat[1] = new LoadT42Format;
		s_fileFormat[2] = new LoadEP1Format;
		s_fileFormat[3] = new LoadHTTFormat;

		s_filters = "All Supported Files (*.";

		for (int i=0; i<s_size; i++) {
			if (i != 0)
				s_filters.append(" *.");
			s_filters.append(s_fileFormat[i]->extensions().join(" *."));
		}
		s_filters.append(");;");

		for (int i=0; i<s_size; i++) {
			if (i != 0)
				s_filters.append(";;");
			s_filters.append(s_fileFormat[i]->fileDialogFilter());
		}
	}

	s_instances++;
}

LoadFormats::~LoadFormats()
{
	s_instances--;

	if (s_instances == 0)
		for (int i=s_size-1; i>=0; i--)
			delete s_fileFormat[i];
}

LoadFormat *LoadFormats::findFormat(const QString &suffix) const
{
	for (int i=0; i<s_size; i++)
		if (s_fileFormat[i]->extensions().contains(suffix, Qt::CaseInsensitive))
			return s_fileFormat[i];

	return nullptr;
}
