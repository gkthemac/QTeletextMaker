/****************************************************************************
** Copyright (c) 2013 Debao Zhang <hello@debao.me>
** All right reserved.
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
****************************************************************************/
#ifndef GIF_IMAGE_Q_GIF_IMAGE_P_H_
#define GIF_IMAGE_Q_GIF_IMAGE_P_H_

#include <giflib/gif_lib.h>
#include <gifimage/qgifimage.h>

#include <QVector>
#include <QColor>

struct QGifFrameInfoData {
  QGifFrameInfoData() : delayTime(-1), interlace(false) {}

public:
  QImage image;
  QPoint offset; //offset info of QImage will lost when convert from One format to another.
  int delayTime;
  bool interlace;
  QColor transparentColor;
};

class QGifImagePrivate {
  Q_DECLARE_PUBLIC(QGifImage)

public:
  explicit QGifImagePrivate(QGifImage *p);
  ~QGifImagePrivate() = default;

public:
  bool load(QIODevice *device);
  bool save(QIODevice *device) const;

  QVector<QRgb> colorTableFromColorMapObject(ColorMapObject *object, int transColorIndex = -1) const;
  [[nodiscard]] ColorMapObject *colorTableToColorMapObject(QVector<QRgb> colorTable) const;

  [[nodiscard]] QSize getCanvasSize() const;
  [[nodiscard]] int getFrameTransparentColorIndex(const QGifFrameInfoData &info) const;

public:
  QSize canvasSize;
  int loopCount;
  int defaultDelayTime;
  QColor defaultTransparentColor;

  QVector<QRgb> globalColorTable;
  QColor bgColor;
  QList<QGifFrameInfoData> frameInfos;

  QGifImage *q_ptr;
};

#endif // GIF_IMAGE_Q_GIF_IMAGE_P_H_
