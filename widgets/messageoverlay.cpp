/*
 * Cantata
 *
 * Copyright (c) 2011-2013 Craig Drummond <craig.p.drummond@gmail.com>
 *
 * ----
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "messageoverlay.h"
#include "toolbutton.h"
#include "icons.h"
#include "localize.h"
#include <QPainter>
#include <QPaintEvent>

MessageOverlay::MessageOverlay(QObject *p)
    : QWidget(0)
{
    Q_UNUSED(p)
    spacing=fontMetrics().height();
    setVisible(false);
    setMinimumHeight(spacing*2);
    setMaximumHeight(spacing*2);
    cancelButton=new ToolButton(this);
    Icon::init(cancelButton);
    cancelButton->setToolTip(i18n("Cancel"));
    Icon icon=Icon("dialog-close");
    if (icon.isNull()) {
        icon=Icon("window-close");
    }
    cancelButton->setIcon(icon);
    cancelButton->adjustSize();
    connect(cancelButton, SIGNAL(clicked()), SIGNAL(cancel()));
}

void MessageOverlay::setWidget(QWidget *widget)
{
    setParent(widget);
    widget->installEventFilter(this);
}

void MessageOverlay::setText(const QString &txt)
{
    if (txt==text) {
        return;
    }

    text=txt;
    setVisible(!text.isEmpty());
    if (!text.isEmpty()) {
        setSizeAndPosition();
        update();
    }
}

static QPainterPath buildPath(const QRectF &r, double radius)
{
    QPainterPath path;
    double diameter(radius*2);

    path.moveTo(r.x()+r.width(), r.y()+r.height()-radius);
    path.arcTo(r.x()+r.width()-diameter, r.y(), diameter, diameter, 0, 90);
    path.arcTo(r.x(), r.y(), diameter, diameter, 90, 90);
    path.arcTo(r.x(), r.y()+r.height()-diameter, diameter, diameter, 180, 90);
    path.arcTo(r.x()+r.width()-diameter, r.y()+r.height()-diameter, diameter, diameter, 270, 90);
    return path;
}

void MessageOverlay::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QRect r(rect());
    QRectF rf(r.x()+0.5, r.y()+0.5, r.width()-1, r.height()-1);
    QColor col=palette().color(QPalette::Highlight);
    col.setAlphaF(0.15);
    p.fillPath(buildPath(rf, r.height()/4.0), col);

    int pad=r.height()/4;
    rf.adjust(pad, pad, -((pad*2)+cancelButton->width()), -pad);

    QFont fnt(font());
    fnt.setBold(true);
    QFontMetrics fm(fnt);
    col=palette().color(QPalette::WindowText);
    col.setAlphaF(0.75);

    p.setPen(col);
    p.setFont(fnt);
    p.drawText(rf, fm.elidedText(text, Qt::ElideRight, r.width(), QPalette::WindowText), QTextOption(Qt::LeftToRight==layoutDirection() ? Qt::AlignLeft : Qt::AlignRight));
    p.end();
}

bool MessageOverlay::eventFilter(QObject *o, QEvent *e)
{
    if (o==parentWidget() && isVisible() && QEvent::Resize==e->type()) {
        setSizeAndPosition();
    }
    return QObject::eventFilter(o, e);
}

void MessageOverlay::setSizeAndPosition()
{
    int currentWidth=width();
    int desiredWidth=parentWidget()->width()-(spacing*2);
    QPoint currentPos=pos();
    QPoint desiredPos=QPoint(spacing, parentWidget()->height()-(height()+spacing));

    if (currentWidth!=desiredWidth) {
        int pad=height()/4;
        resize(desiredWidth, height());
        cancelButton->move(QPoint(Qt::LeftToRight==layoutDirection()
                                    ? desiredWidth-(cancelButton->width()+pad)
                                    : pad,
                                  (height()-cancelButton->height())/2));
    }

    if (currentPos!=desiredPos) {
        move(desiredPos);
    }
}
