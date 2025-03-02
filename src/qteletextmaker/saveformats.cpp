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

#include "saveformats.h"

#include <QByteArray>
#include <QDataStream>
#include <QSaveFile>
#include <QString>

#include "document.h"
#include "hamming.h"
#include "levelonepage.h"
#include "pagebase.h"

void SaveFormat::saveAllPages(QSaveFile &outFile, const TeletextDocument &document)
{
	m_document = &document;
	m_outFile = &outFile;
	m_outStream.setDevice(m_outFile);
	m_warnings.clear();
	m_error.clear();

	writeDocumentStart();
	writeAllPages();
	writeDocumentEnd();
}

void SaveFormat::saveCurrentSubPage(QSaveFile &outFile, const TeletextDocument &document)
{
	m_document = &document;
	m_outFile = &outFile;
	m_outStream.setDevice(m_outFile);
	m_warnings.clear();
	m_error.clear();

	writeDocumentStart();
	writeSubPage(*m_document->currentSubPage());
	writeDocumentEnd();
}

void SaveFormat::writeAllPages()
{
	for (int p=0; p<m_document->numberOfSubPages(); p++)
		writeSubPage(*m_document->subPage(p), (m_document->numberOfSubPages() == 1) ? 0 : p+1);
}

void SaveFormat::writeSubPage(const PageBase &subPage, int subPageNumber)
{
	writeSubPageStart(subPage, subPageNumber);
	writeSubPageBody(subPage);
	writeSubPageEnd(subPage);
}

void SaveFormat::writeSubPageBody(const PageBase &subPage)
{
	writeX27Packets(subPage);
	writeX28Packets(subPage);
	if (m_document->pageFunction() == TeletextDocument::PFLevelOnePage) {
		writeX26Packets(subPage);
		writeX1to25Packets(subPage);
	} else {
		qDebug("Not LevelOnePage, assuming 7-bit packets!");
		writeX1to25Packets(subPage);
		writeX26Packets(subPage);
	}
}

void SaveFormat::writeX27Packets(const PageBase &subPage)
{
	for (int i=0; i<4; i++)
		if (subPage.packetExists(27, i))
			writePacket(format4BitPacket(subPage.packet(27, i)), 27, i);
	for (int i=4; i<16; i++)
		if (subPage.packetExists(27, i))
			writePacket(format18BitPacket(subPage.packet(27, i)), 27, i);
}

void SaveFormat::writeX28Packets(const PageBase &subPage)
{
	for (int i=0; i<16; i++)
		if (subPage.packetExists(28, i))
			writePacket(format18BitPacket(subPage.packet(28, i)), 28, i);
}

void SaveFormat::writeX26Packets(const PageBase &subPage)
{
	for (int i=0; i<16; i++)
		if (subPage.packetExists(26, i))
			writePacket(format18BitPacket(subPage.packet(26, i)), 26, i);
}

void SaveFormat::writeX1to25Packets(const PageBase &subPage)
{
	for (int i=1; i<26; i++)
		if (subPage.packetExists(i))
			// BUG must check m_document->packetCoding() !!
			writePacket(format7BitPacket(subPage.packet(i)), i);
}

int SaveFormat::writePacket(QByteArray packet, int packetNumber, int designationCode)
{
	return writeRawData(packet, packet.size());
}

int SaveFormat::writeRawData(const char *s, int len)
{
	return m_outStream.writeRawData(s, len);
}


inline void SaveTTIFormat::writeString(const QString &command)
{
	QByteArray result = command.toUtf8() + "\r\n";

	writeRawData(result, result.size());
}

void SaveTTIFormat::writeDocumentStart()
{
	if (!m_document->description().isEmpty())
		writeString(QString("DE,%1").arg(m_document->description()));

	// TODO DS and SP commands
}

QByteArray SaveTTIFormat::format7BitPacket(QByteArray packet)
{
	for (int i=0; i<packet.size(); i++)
		if (packet.at(i) < 0x20) {
			// TTI files are plain text, so put in escape followed by control code with bit 6 set
			packet[i] = packet.at(i) | 0x40;
			packet.insert(i, 0x1b);
			i++;
		}

	return packet;
}

// format4BitPacket in this class calls this method as the encoding is the same
// i.e. the bytes as-is with bit 6 set to make them ASCII friendly
QByteArray SaveTTIFormat::format18BitPacket(QByteArray packet)
{
	// TTI stores the triplets 6 bits at a time like we do, without Hamming encoding
	// We don't touch the first byte; the caller replaces it with the designation code
	// unless it's X/1-X/25 used in (G)POP pages
	for (int i=1; i<packet.size(); i++)
		packet[i] = packet.at(i) | 0x40;

	return packet;
}

void SaveTTIFormat::writeSubPageStart(const PageBase &subPage, int subPageNumber)
{
	// Page number
	writeString(QString("PN,%1%2").arg(m_document->pageNumber(), 3, 16, QChar('0')).arg(subPageNumber & 0xff, 2, 10, QChar('0')));

	// Subpage
	// Magazine Organisation Table and Magazine Inventory Page don't have subpages
	if (m_document->pageFunction() != TeletextDocument::PFMOT && m_document->pageFunction() != TeletextDocument::PFMIP)
		writeString(QString("SC,%1").arg(subPageNumber, 4, 10, QChar('0')));

	// Status bits
	// We assume that bit 15 "transmit page" is always wanted
	// C4 stored in bit 14
	int statusBits = 0x8000 | (subPage.controlBit(PageBase::C4ErasePage) << 14);
	// C5 to C11 stored in order from bits 1 to 6
	for (int i=PageBase::C5Newsflash; i<=PageBase::C11SerialMagazine; i++)
		statusBits |= subPage.controlBit(i) << (i-1);
	// NOS in bits 7 to 9
	statusBits |= subPage.controlBit(PageBase::C12NOS) << 9;
	statusBits |= subPage.controlBit(PageBase::C13NOS) << 8;
	statusBits |= subPage.controlBit(PageBase::C14NOS) << 7;

	writeString(QString("PS,%1").arg(0x8000 | statusBits, 4, 16, QChar('0')));

	if (m_document->pageFunction() == TeletextDocument::PFLevelOnePage)
		writeString(QString("CT,%1,%2").arg(static_cast<const LevelOnePage *>(&subPage)->cycleValue()).arg(static_cast<const LevelOnePage *>(&subPage)->cycleType()==LevelOnePage::CTcycles ? 'C' : 'T'));
	else
		// X/28/0 specifies page function and coding but the PF command
		// should make it obvious to a human that this is not a Level One Page
		writeString(QString("PF,%1,%2").arg(m_document->pageFunction()).arg(m_document->packetCoding()));
}

void SaveTTIFormat::writeSubPageBody(const PageBase &subPage)
{
	// FLOF links
	bool writeFLCommand = false;
	if (m_document->pageFunction() == TeletextDocument::PFLevelOnePage && subPage.packetExists(27,0)) {
		// Subpage has FLOF links - if any link to a specific subpage we need to write X/27/0
		// as raw, otherwise we write the links as a human-readable FL command later on
		writeFLCommand = true;
		// TODO uncomment this when we can edit FLOF subpage links
		/*for (int i=0; i<6; i++)
			if (document.subPage(p)->fastTextLinkSubPageNumber(i) != 0x3f7f) {
				writeFLCommand = false;
				break;
			}*/
	}

	// Don't write X/27/0 if FL command will be written
	// but write the rest of the X/27 packets
	for (int i=(writeFLCommand ? 1 : 0); i<4; i++)
		if (subPage.packetExists(27, i))
			writePacket(format4BitPacket(subPage.packet(27, i)), 27, i);
	for (int i=4; i<16; i++)
		if (subPage.packetExists(27, i))
			writePacket(format18BitPacket(subPage.packet(27, i)), 27, i);

	// Now write the other packets like the rest of writeSubPageBody does
	writeX28Packets(subPage);
	if (m_document->pageFunction() == TeletextDocument::PFLevelOnePage) {
		writeX26Packets(subPage);
		writeX1to25Packets(subPage);
	} else {
		qDebug("Not LevelOnePage, assuming 7-bit packets!");
		writeX1to25Packets(subPage);
		writeX26Packets(subPage);
	}

	if (writeFLCommand) {
		QString flofLine;
		flofLine.reserve(26);
		flofLine="FL,";

		for (int i=0; i<6; i++) {
			// Stored as page link with relative magazine number, convert to absolute page number for display
			int absoluteLinkPageNumber = static_cast<const LevelOnePage *>(&subPage)->fastTextLinkPageNumber(i) ^ (m_document->pageNumber() & 0x700);
			// Fix magazine 0 to 8
			if ((absoluteLinkPageNumber & 0x700) == 0x000)
				absoluteLinkPageNumber |= 0x800;

			flofLine.append(QString("%1").arg(absoluteLinkPageNumber, 3, 16, QChar('0')));
			if (i < 5)
				flofLine.append(',');
		}

		writeString(flofLine);
	}
}

int SaveTTIFormat::writePacket(QByteArray packet, int packetNumber, int designationCode)
{
	if (designationCode != -1)
		packet[0] = designationCode | 0x40;

	packet.prepend("OL," + QByteArray::number(packetNumber) + ",");
	packet.append("\r\n");

	return(writeRawData(packet, packet.size()));
}


void SaveM29Format::writeSubPageStart(const PageBase &subPage, int subPageNumber)
{
	Q_UNUSED(subPageNumber);

	// Force page number to 0xFF and subpage to 0
	writeString(QString("PN,%1ff00").arg(m_document->pageNumber() >> 8, 1, 16));
	// Not sure if this PS forcing is necessary
	writeString("PS,8000");
}

void SaveM29Format::writeSubPageBody(const PageBase &subPage)
{
	if (subPage.packetExists(28, 0))
		writePacket(format18BitPacket(subPage.packet(28, 0)), 29, 0);
	if (subPage.packetExists(28, 1))
		writePacket(format18BitPacket(subPage.packet(28, 1)), 29, 1);
	if (subPage.packetExists(28, 4))
		writePacket(format18BitPacket(subPage.packet(28, 4)), 29, 4);
}


QByteArray SaveT42Format::format7BitPacket(QByteArray packet)
{
	// Odd parity encoding
	for (int c=0; c<packet.size(); c++) {
		char p = packet.at(c);

		// Recursively divide integer into two equal halves and take their XOR until only 1 bit is left
		p ^= p >> 4;
		p ^= p >> 2;
		p ^= p >> 1;
		// If last bit left is 0 then it started with an even number of bits, so do the odd parity
		if (!(p & 1))
			packet[c] = packet.at(c) | 0x80;
	}

	return packet;
}

QByteArray SaveT42Format::format4BitPacket(QByteArray packet)
{
	for (int c=0; c<packet.size(); c++)
		packet[c] = hamming_8_4_encode[(int)packet.at(c)];

	return packet;
}

QByteArray SaveT42Format::format18BitPacket(QByteArray packet)
{
	for (int c=1; c<packet.size(); c+=3) {
		unsigned int D5_D11;
		unsigned int D12_D18;
		unsigned int P5, P6;
		unsigned int Byte_0;

		const unsigned int toEncode = packet[c] | (packet[c+1] << 6) | (packet[c+2] << 12);

		Byte_0 = (hamming_24_18_forward[0][(toEncode >> 0) & 0xff] ^ hamming_24_18_forward[1][(toEncode >> 8) & 0xff] ^ hamming_24_18_forward_2[(toEncode >> 16) & 0x03]);
		packet[c] = Byte_0;

		D5_D11 = (toEncode >> 4) & 0x7f;
		D12_D18 = (toEncode >> 11) & 0x7f;

		P5 = 0x80 & ~(hamming_24_18_parities[0][D12_D18] << 2);
		packet[c+1] = D5_D11 | P5;

		P6 = 0x80 & ((hamming_24_18_parities[0][Byte_0] ^ hamming_24_18_parities[0][D5_D11]) << 2);
		packet[c+2] = D12_D18 | P6;
	}

	return packet;
}

int SaveT42Format::writePacket(QByteArray packet, int packetNumber, int designationCode)
{
	// Byte 2 - designation code
	if (designationCode != - 1)
		packet[0] = hamming_8_4_encode[designationCode];

	// Byte 1 of MRAG
	packet.prepend(hamming_8_4_encode[packetNumber >> 1]);
	// Byte 0 of MRAG
	packet.prepend(hamming_8_4_encode[m_magazineNumber | ((packetNumber & 0x01) << 3)]);

	return(writeRawData(packet, packet.size()));
}

void SaveT42Format::writeSubPageStart(const PageBase &subPage, int subPageNumber)
{
	// Convert integer to Binary Coded Decimal
	subPageNumber = QString::number(subPageNumber).toInt(nullptr, 16);

	// Displayable row header we export as spaces, hence the (odd parity valid) 0x20 init value
	QByteArray packet(42, 0x20);

	m_magazineNumber = (m_document->pageNumber() & 0xf00) >> 8;
	if (m_magazineNumber == 8)
		m_magazineNumber = 0;

	// Write X/0 separately as it features both Hamming 8/4 and 7-bit odd parity within
	packet[0] = m_magazineNumber & 0x07;
	packet[1] = 0; // Packet number 0
	packet[2] = m_document->pageNumber() & 0x00f;
	packet[3] = (m_document->pageNumber() & 0x0f0) >> 4;
	packet[4] = subPageNumber & 0xf;
	packet[5] = ((subPageNumber >> 4) & 0x7) | (subPage.controlBit(PageBase::C4ErasePage) << 3);
	packet[6] = ((subPageNumber >> 8) & 0xf);
	packet[7] = ((subPageNumber >> 12) & 0x3) | (subPage.controlBit(PageBase::C5Newsflash) << 2) | (subPage.controlBit(PageBase::C6Subtitle) << 3);
	packet[8] = subPage.controlBit(PageBase::C7SuppressHeader) | (subPage.controlBit(PageBase::C8Update) << 1) | (subPage.controlBit(PageBase::C9InterruptedSequence) << 2) | (subPage.controlBit(PageBase::C10InhibitDisplay) << 3);
	packet[9] = subPage.controlBit(PageBase::C11SerialMagazine) | (subPage.controlBit(PageBase::C14NOS) << 1) | (subPage.controlBit(PageBase::C13NOS) << 2) | (subPage.controlBit(PageBase::C12NOS) << 3);

	for (int i=0; i<10; i++)
		packet[i] = hamming_8_4_encode[(int)packet.at(i)];

	writeRawData(packet.constData(), 42);
}


int SaveHTTFormat::writeRawData(const char *s, int len)
{
	char httLine[45];

	httLine[0] = 0xaa;
	httLine[1] = 0xaa;
	httLine[2] = 0xe4;

	for (int i=0; i<42; i++) {
		unsigned char b = s[i];
		b = (b & 0xf0) >> 4 | (b & 0x0f) << 4;
		b = (b & 0xcc) >> 2 | (b & 0x33) << 2;
		b = (b & 0xaa) >> 1 | (b & 0x55) << 1;
		httLine[i+3] = b;
	}

	return m_outStream.writeRawData(httLine, len+3);
}


bool SaveEP1Format::getWarnings(const PageBase &subPage)
{
	m_warnings.clear();

	if (!m_languageCode.contains((static_cast<const LevelOnePage *>(&subPage)->defaultCharSet() << 3) | static_cast<const LevelOnePage *>(&subPage)->defaultNOS()))
		m_warnings.append("Page language not supported, will be exported as English.");

	if (subPage.packetExists(24) || subPage.packetExists(27, 0))
		m_warnings.append("FLOF display row and page links will not be exported.");

	if (subPage.packetExists(27, 4) || subPage.packetExists(27, 5))
		m_warnings.append("X/27/4-5 compositional links will not be exported.");

	if (subPage.packetExists(28, 0) || subPage.packetExists(28, 4))
		m_warnings.append("X/28 page enhancements will not be exported.");

	return (!m_warnings.isEmpty());
}

QByteArray SaveEP1Format::format18BitPacket(QByteArray packet)
{
	for (int c=1; c<packet.size(); c+=3) {
		// Shuffle triplet bits to 6 bit address, 5 bit mode, 7 bit data
		packet[c+2] = ((packet.at(c+2) & 0x3f) << 1) | ((packet.at(c+1) & 0x20) >> 5);
		packet[c+1] = packet.at(c+1) & 0x1f;
		// Address of termination marker is 7f instead of 3f
		if (packet.at(c+1) == 0x1f && packet.at(c) == 0x3f)
			packet[c] = 0x7f;
	}

	return packet;
}

void SaveEP1Format::writeSubPageStart(const PageBase &subPage, int subPageNumber)
{
	Q_UNUSED(subPageNumber);

	QByteArray pageStart(3, 0x00);

	// First two bytes always 0xfe, 0x01
	pageStart[0] = 0xfe;
	pageStart[1] = 0x01;
	// Next byte is language code unique to EP1
	// Unknown values are mapped to English, after warning the user
	pageStart[2] = m_languageCode.value((static_cast<const LevelOnePage *>(&subPage)->defaultCharSet() << 3) | static_cast<const LevelOnePage *>(&subPage)->defaultNOS(), 0x09);

	writeRawData(pageStart.constData(), 3);
}

void SaveEP1Format::writeSubPageBody(const PageBase &subPage)
{
	// Following the first three bytes already written by writeSubPageStart,
	// the next three bytes for pages with X/26 packets are 0xca
	// then little-endian offset to start of Level 1 page data.
	// For pages with no X/26 packets, just three zeroes.
	QByteArray offsetData(4, 0x00);

	int numOfX26Packets = 0;

	if (subPage.packetExists(26, 0)) {
		offsetData[0] = 0xca;

		while (subPage.packetExists(26, numOfX26Packets))
			numOfX26Packets++;

		const int level1Offset = numOfX26Packets * 40 + 4;
		offsetData[1] = level1Offset & 0xff;
		offsetData[2] = level1Offset >> 8;
	}

	writeRawData(offsetData.constData(), 3);

	// We should really re-implement writeX26Packets but I can't be bothered
	// to count the number of X/26 packets twice...
	if (numOfX26Packets > 0) {
		// Reuse offsetData for this 4-byte header of the enhancement data
		// Bytes are 0xc2, 0x00, then little-endian length of enhancement data
		offsetData[0] = 0xc2;
		offsetData[1] = 0x00;
		offsetData[2] = (numOfX26Packets * 40) & 0xff;
		offsetData[3] = (numOfX26Packets * 40) >> 8;
		writeRawData(offsetData.constData(), 4);

		for (int i=0; i<numOfX26Packets; i++) {
			QByteArray packet = format18BitPacket(subPage.packet(26, i));
			packet[0] = i;
			writeRawData(packet.constData(), 40);
		}
	}

	writeX1to25Packets(subPage);
}

void SaveEP1Format::writeSubPageEnd(const PageBase &subPage)
{
	// 40 byte buffer for undo purposes or something? Just write a blank row of spaces
	writeRawData(QByteArray(40, 0x20).constData(), 40);

	// Last two bytes always 0x00, 0x00
	writeRawData(QByteArray(2, 0x00).constData(), 2);
}

void SaveEP1Format::writeX1to25Packets(const PageBase &subPage)
{
	for (int i=0; i<24; i++)
		if (subPage.packetExists(i))
			writePacket(format7BitPacket(subPage.packet(i)), i, 0);
		else
			writeRawData(QByteArray(40, 0x20).constData(), 40);
}


int SaveFormats::s_instances = 0;

SaveFormats::SaveFormats()
{
	if (s_instances == 0) {
		s_fileFormat[0] = new SaveTTIFormat;
		s_fileFormat[1] = new SaveT42Format;
		s_fileFormat[2] = new SaveEP1Format;
		s_fileFormat[3] = new SaveHTTFormat;

		for (int i=0; i<s_size; i++) {
			if (i != 0)
				s_exportFilters.append(";;");
			s_exportFilters.append(s_fileFormat[i]->fileDialogFilter());
			if (i < s_nativeSize) {
				if (i != 0)
					s_filters.append(";;");
				s_filters.append(s_fileFormat[i]->fileDialogFilter());
			}
		}
	}

	s_instances++;
}

SaveFormats::~SaveFormats()
{
	s_instances--;

	if (s_instances == 0)
		for (int i=s_size-1; i>=0; i--)
			delete s_fileFormat[i];
}

SaveFormat *SaveFormats::findFormat(const QString &suffix) const
{
	// TTI is the only official save format at the moment...
//	for (int i=0; i<s_nativeSize; i++)
//		if (s_fileFormat[i]->extensions().contains(suffix, Qt::CaseInsensitive))
//			return s_fileFormat[i];

	if (s_fileFormat[0]->extensions().contains(suffix, Qt::CaseInsensitive))
		return s_fileFormat[0];

	return nullptr;
}

SaveFormat *SaveFormats::findExportFormat(const QString &suffix) const
{
	for (int i=0; i<s_size; i++)
		if (s_fileFormat[i]->extensions().contains(suffix, Qt::CaseInsensitive))
			return s_fileFormat[i];

	return nullptr;
}
