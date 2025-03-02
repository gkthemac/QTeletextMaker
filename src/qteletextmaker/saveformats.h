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

#ifndef SAVEFORMATS_H
#define SAVEFORMATS_H

#include <QByteArray>
#include <QDataStream>
#include <QSaveFile>
#include <QString>

#include "document.h"
#include "levelonepage.h"
#include "pagebase.h"

class SaveFormat
{
public:
	virtual ~SaveFormat() { };

	virtual void saveAllPages(QSaveFile &outFile, const TeletextDocument &document);
	virtual void saveCurrentSubPage(QSaveFile &outFile, const TeletextDocument &document);

	virtual QString description() const =0;
	virtual QStringList extensions() const =0;
	QString fileDialogFilter() const { return QString(description() + " (*." + extensions().join(" *.") + ')'); };
	virtual bool getWarnings(const PageBase &subPage) { return false; };
	QStringList warningStrings() const { return m_warnings; };
	QString errorString() const { return m_error; };

protected:
	virtual void writeDocumentStart() { };
	virtual void writeAllPages();
	virtual void writeSubPage(const PageBase &subPage, int subPageNumber=0);
	virtual void writeSubPageStart(const PageBase &subPage, int subPageNumber=0) { };
	virtual void writeSubPageBody(const PageBase &subPage);
	virtual void writeSubPageEnd(const PageBase &subPage) { };
	virtual void writeDocumentEnd() { };

	virtual void writeX27Packets(const PageBase &subPage);
	virtual void writeX28Packets(const PageBase &subPage);
	virtual void writeX26Packets(const PageBase &subPage);
	virtual void writeX1to25Packets(const PageBase &subPage);

	virtual QByteArray format7BitPacket(QByteArray packet) { return packet; };
	virtual QByteArray format4BitPacket(QByteArray packet) { return packet; };
	virtual QByteArray format18BitPacket(QByteArray packet) { return packet; };

	virtual int writePacket(QByteArray packet, int packetNumber, int designationCode = -1);
	virtual int writeRawData(const char *s, int len);

	TeletextDocument const *m_document;
	QSaveFile *m_outFile;
	QDataStream m_outStream;
	QStringList m_warnings;
	QString m_error;
};

class SaveTTIFormat : public SaveFormat
{
public:
	QString description() const override { return QString("MRG Systems TTI"); };
	QStringList extensions() const override { return QStringList { "tti", "ttix" }; };

protected:
	virtual void writeDocumentStart();
	virtual void writeSubPageStart(const PageBase &subPage, int subPageNumber=0);
	virtual void writeSubPageBody(const PageBase &subPage);

	virtual int writePacket(QByteArray packet, int packetNumber, int designationCode = -1);
	void writeString(const QString &command);

	QByteArray format7BitPacket(QByteArray packet);
	QByteArray format4BitPacket(QByteArray packet) { return format18BitPacket(packet); };
	QByteArray format18BitPacket(QByteArray packet);
};

class SaveM29Format : public SaveTTIFormat
{
protected:
	void writeSubPageStart(const PageBase &subPage, int subPageNumber=0);
	void writeSubPageBody(const PageBase &subPage);
};

class SaveT42Format : public SaveFormat
{
public:
	QString description() const override { return QString("t42 packet stream"); };
	QStringList extensions() const override { return QStringList { "t42" }; };

protected:
	void writeSubPageStart(const PageBase &subPage, int subPageNumber=0);
	virtual int writePacket(QByteArray packet, int packetNumber, int designationCode = -1);

	virtual QByteArray format7BitPacket(QByteArray packet);
	virtual QByteArray format4BitPacket(QByteArray packet);
	virtual QByteArray format18BitPacket(QByteArray packet);

	int m_magazineNumber;
};

class SaveHTTFormat : public SaveT42Format
{
public:
	QString description() const override { return QString("HMS SD-Teletext HTT"); };
	QStringList extensions() const override { return QStringList { "htt" }; };

protected:
	int writeRawData(const char *s, int len) override;
};

class SaveEP1Format : public SaveFormat
{
public:
	QString description() const override { return QString("Softel EP1"); };
	QStringList extensions() const override { return QStringList { "ep1" }; };
	virtual bool getWarnings(const PageBase &subPage);

protected:
	void writeSubPageStart(const PageBase &subPage, int subPageNumber=0);
	void writeSubPageBody(const PageBase &subPage);
	void writeSubPageEnd(const PageBase &subPage);

	virtual void writeX1to25Packets(const PageBase &subPage);

	virtual QByteArray format18BitPacket(QByteArray packet);

	// Language codes unique to EP1
	// FIXME duplicated in loadformats.h
	const QMap<int, int> m_languageCode {
		{ 0x00, 0x09 }, { 0x01, 0x0d }, { 0x02, 0x18 }, { 0x03, 0x11 }, { 0x04, 0x0b }, { 0x05, 0x17 }, { 0x06, 0x07 },
		{ 0x08, 0x14 }, { 0x09, 0x0d }, { 0x0a, 0x18 }, { 0x0b, 0x11 }, { 0x0c, 0x0b }, { 0x0e, 0x07 },
		{ 0x10, 0x09 }, { 0x11, 0x0d }, { 0x12, 0x18 }, { 0x13, 0x11 }, { 0x14, 0x0b }, { 0x15, 0x17 }, { 0x16, 0x1c },
		{ 0x1d, 0x1e }, { 0x1f, 0x16 },
		{ 0x21, 0x0d }, { 0x22, 0xff }, { 0x23, 0xff }, { 0x26, 0x07 },
		{ 0x36, 0x1c }, { 0x37, 0x0e },
		{ 0x40, 0x09 }, { 0x44, 0x0b }
	};
};


class SaveFormats
{
public:
	SaveFormats();
	~SaveFormats();

	SaveFormat *findFormat(const QString &suffix) const;
	SaveFormat *findExportFormat(const QString &suffix) const;
	QString filters() const { return s_filters; };
	QString exportFilters() const { return s_exportFilters; };
	bool isExportOnly(const QString &suffix) const { return findFormat(suffix) == nullptr; };

private:
	static const inline int s_size = 4;
	static const inline int s_nativeSize = 1;
	static int s_instances;
	inline static SaveFormat *s_fileFormat[s_size];
	inline static QString s_filters, s_exportFilters;
};

#endif
