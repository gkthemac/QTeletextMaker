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

#ifndef LOADFORMATS_H
#define LOADFORMATS_H

#include <QByteArray>
#include <QDataStream>
#include <QFile>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "pagebase.h"

class LoadFormat
{
public:
	virtual ~LoadFormat() {};

	virtual bool load(QFile *inFile, QList<PageBase> &subPages, QVariantHash *metadata = nullptr) =0;

	virtual QString description() const =0;
	virtual QStringList extensions() const =0;
	QString fileDialogFilter() const { return QString(description() + " (*." + extensions().join(" *.") + ')'); };
	QStringList warningStrings() const { return m_warnings; };
	QString errorString() const { return m_error; };
	bool reExportWarning() const { return m_reExportWarning; };

protected:
	QStringList m_warnings;
	QString m_error;
	bool m_reExportWarning = false;
};

class LoadTTIFormat : public LoadFormat
{
public:
	bool load(QFile *inFile, QList<PageBase> &subPages, QVariantHash *metadata = nullptr) override;

	QString description() const override { return QString("MRG Systems TTI"); };
	QStringList extensions() const override { return QStringList { "tti", "ttix" }; };
};

class LoadT42Format : public LoadFormat
{
public:
	bool load(QFile *inFile, QList<PageBase> &subPages, QVariantHash *metadata = nullptr) override;

	QString description() const override { return QString("t42 packet stream"); };
	QStringList extensions() const override { return QStringList { "t42" }; };

protected:
	virtual bool readPacket();

	QFile *m_inFile;
	unsigned char m_inLine[42];
};

class LoadHTTFormat : public LoadT42Format
{
public:
	QString description() const override { return QString("HMS SD-Teletext HTT"); };
	QStringList extensions() const override { return QStringList { "htt" }; };

protected:
	bool readPacket() override;
};

class LoadEP1Format : public LoadFormat
{
public:
	bool load(QFile *inFile, QList<PageBase> &subPages, QVariantHash *metadata = nullptr) override;

	QString description() const override { return QString("Softel EP1"); };
	QStringList extensions() const override { return QStringList { "ep1", "epx" }; };

protected:
	// Language codes unique to EP1
	// FIXME duplicated in saveformats.h
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


class LoadFormats
{
public:
	LoadFormats();
	~LoadFormats();

	LoadFormat *findFormat(const QString &suffix) const;
	QString filters() const { return s_filters; };

private:
	static const inline int s_size = 4;
	static int s_instances;
	inline static LoadFormat *s_fileFormat[s_size];
	inline static QString s_filters;
};

#endif
