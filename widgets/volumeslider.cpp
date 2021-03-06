/*
 * Cantata
 *
 * Copyright (c) 2011-2014 Craig Drummond <craig.p.drummond@gmail.com>
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

#include "volumeslider.h"
#include "support/localize.h"
#include "mpd-interface/mpdconnection.h"
#include "mpd-interface/mpdstatus.h"
#include "support/action.h"
#include "support/actioncollection.h"
#include "gui/stdactions.h"
#include "support/utils.h"
#include "gui/settings.h"
#include <QStyle>
#include <QPainter>
#include <QPainterPath>
#include <QProxyStyle>
#include <QApplication>
#include <QLabel>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMenu>
#ifdef ENABLE_KDE_SUPPORT
#include <KDE/KShortcut>
#endif

class VolumeSliderProxyStyle : public QProxyStyle
{
public:
    VolumeSliderProxyStyle()
        : QProxyStyle()
    {
        setBaseStyle(qApp->style());
    }

    int styleHint(StyleHint stylehint, const QStyleOption *opt, const QWidget *widget, QStyleHintReturn *returnData) const
    {
        if (SH_Slider_AbsoluteSetButtons==stylehint) {
            return Qt::LeftButton|QProxyStyle::styleHint(stylehint, opt, widget, returnData);
        } else {
            return QProxyStyle::styleHint(stylehint, opt, widget, returnData);
        }
    }
};


static int widthStep=4;
static int constHeightStep=2;

QColor VolumeSlider::clampColor(const QColor &col)
{
    static const int constMin=64;
    static const int constMax=240;

    if (col.value()<constMin) {
        return QColor(constMin, constMin, constMin);
    } else if (col.value()>constMax) {
        return QColor(constMax, constMax, constMax);
    }
    return col;
}

VolumeSlider::VolumeSlider(QWidget *p)
    : QSlider(p)
    , lineWidth(0)
    , shown(false)
    , down(false)
    , fadingStop(false)
    , muteAction(0)
    , menu(0)
{
    widthStep=Utils::touchFriendly() ? 5 : 4;
    setRange(0, 100);
    setPageStep(Settings::self()->volumeStep());
    lineWidth=Utils::scaleForDpi(1);

    int w=lineWidth*widthStep*19;
    int h=lineWidth*constHeightStep*10;
    setFixedHeight(h+1);
    setFixedWidth(w);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setOrientation(Qt::Horizontal);
    setFocusPolicy(Qt::NoFocus);
    setStyle(new VolumeSliderProxyStyle());
    setStyleSheet(QString("QSlider::groove:horizontal {border: 0px;} "
                          "QSlider::sub-page:horizontal {border: 0px;} "
                          "QSlider::handle:horizontal {width: 0px; height:0px; margin:0;}"));
    textCol=clampColor(palette().color(QPalette::Active, QPalette::Text));
    generatePixmaps();
}

void VolumeSlider::initActions()
{
    if (muteAction) {
        return;
    }
    muteAction = ActionCollection::get()->createAction("mute", i18n("Mute"));
    addAction(muteAction);
    #ifdef ENABLE_KDE_SUPPORT
    muteAction->setGlobalShortcut(KShortcut(Qt::Key_VolumeMute));
    StdActions::self()->increaseVolumeAction->setGlobalShortcut(KShortcut(Qt::Key_VolumeUp));
    StdActions::self()->decreaseVolumeAction->setGlobalShortcut(KShortcut(Qt::Key_VolumeDown));
    #endif
    connect(muteAction, SIGNAL(triggered()), MPDConnection::self(), SLOT(toggleMute()));
    connect(MPDStatus::self(), SIGNAL(updated()), this, SLOT(updateMpdStatus()));
    connect(StdActions::self()->increaseVolumeAction, SIGNAL(triggered()), this, SLOT(increaseVolume()));
    connect(StdActions::self()->decreaseVolumeAction, SIGNAL(triggered()), this, SLOT(decreaseVolume()));
    connect(this, SIGNAL(valueChanged(int)), MPDConnection::self(), SLOT(setVolume(int)));
    addAction(StdActions::self()->increaseVolumeAction);
    addAction(StdActions::self()->decreaseVolumeAction);
}

void VolumeSlider::showEvent(QShowEvent *ev)
{
    if (!shown) {
        shown=true;
        QLabel lbl(parentWidget());
        lbl.ensurePolished();
        QColor col=clampColor(lbl.palette().text().color());

        if (col!=textCol) {
            textCol=col;
            generatePixmaps();
        }
    }
    QSlider::showEvent(ev);
}

void VolumeSlider::paintEvent(QPaintEvent *)
{
    bool reverse=isRightToLeft();
    QPainter p(this);
    bool muted=MPDConnection::self()->isMuted();
    if (muted || !isEnabled()) {
        p.setOpacity(0.25);
    }

    p.drawPixmap(0, 0, pixmaps[0]);
    #if 1
    int steps=(value()/10.0)+0.5;
    if (steps>0) {
        if (steps<10) {
            int wStep=widthStep*lineWidth;
            p.setClipRect(reverse
                            ? QRect(width()-((steps*wStep*2)-wStep), 0, width(), height())
                            : QRect(0, 0, (steps*wStep*2)-wStep, height()));
            p.setClipping(true);
        }
        p.drawPixmap(0, 0, pixmaps[1]);
        if (steps<10) {
            p.setClipping(false);
        }
    }
    #else // Partial filling of each block?
    if (value()>0) {
        if (value()<100) {
            int fillWidth=(width()*(0.01*value()))+0.5;
            p.setClipRect(reverse
                            ? QRect(width()-fillWidth, 0, width(), height())
                            : QRect(0, 0, fillWidth, height()));
            p.setClipping(true);
        }
        p.drawPixmap(0, 0, *(pixmaps[1]));
        if (value()<100) {
            p.setClipping(false);
        }
    }
    #endif

    if (!muted) {
        p.setOpacity(p.opacity()*0.75);
        p.setPen(textCol);
        QFont f(font());
        f.setPixelSize(qMax(height()/2.5, 8.0));
        p.setFont(f);
        QRect r=rect();
        bool rtl=isRightToLeft();
        if (rtl) {
            r.setX(widthStep*lineWidth*12);
        } else {
            r.setWidth(widthStep*lineWidth*7);
        }
        p.drawText(r, Qt::AlignRight, QString("%1%").arg(value()));
    }
}

void VolumeSlider::mousePressEvent(QMouseEvent *ev)
{
    if (Qt::MiddleButton==ev->buttons()) {
        down=true;
    } else {
        QSlider::mousePressEvent(ev);
    }
}

void VolumeSlider::mouseReleaseEvent(QMouseEvent *ev)
{
    if (down) {
        down=false;
        muteAction->trigger();
        update();
    } else {
        QSlider::mouseReleaseEvent(ev);
    }
}

void VolumeSlider::contextMenuEvent(QContextMenuEvent *ev)
{
    static const char *constValProp="val";
    if (!menu) {
        menu=new QMenu(this);
        muteMenuAction=menu->addAction(i18n("Mute"));
        muteMenuAction->setProperty(constValProp, -1);
        for (int i=0; i<11; ++i) {
            menu->addAction(QString("%1%").arg(i*10))->setProperty(constValProp, i*10);
        }
    }

    muteMenuAction->setText(MPDConnection::self()->isMuted() ? i18n("Unmute") : i18n("Mute"));
    QAction *ret = menu->exec(mapToGlobal(ev->pos()));
    if (ret) {
        int val=ret->property(constValProp).toInt();
        if (-1==val) {
            muteAction->trigger();
        } else {
            setValue(val);
        }
    }
}

void VolumeSlider::wheelEvent(QWheelEvent *ev)
{
    int numDegrees = ev->delta() / 8;
    int numSteps = numDegrees / 15;
    if (numSteps > 0) {
        for (int i = 0; i < numSteps; ++i) {
            increaseVolume();
        }
    } else {
        for (int i = 0; i > numSteps; --i) {
            decreaseVolume();
        }
    }
}

void VolumeSlider::updateMpdStatus()
{
    if (fadingStop) {
        return;
    }

    int volume=MPDStatus::self()->volume();

    blockSignals(true);
    if (volume<0) {
        setValue(0);
    } else {
        int unmuteVolume=-1;
        if (0==volume) {
            unmuteVolume=MPDConnection::self()->unmuteVolume();
            if (unmuteVolume>0) {
                volume=unmuteVolume;
            }
        }
        setEnabled(true);
        setToolTip(unmuteVolume>0 ? i18n("Volume %1% (Muted)", volume) : i18n("Volume %1%", volume));
        setValue(volume);
    }
    setEnabled(volume>=0);
    setVisible(volume>=0);
    update();
    muteAction->setEnabled(isEnabled());
    StdActions::self()->increaseVolumeAction->setEnabled(isEnabled());
    StdActions::self()->decreaseVolumeAction->setEnabled(isEnabled());
    blockSignals(false);
}

void VolumeSlider::increaseVolume()
{
    triggerAction(QAbstractSlider::SliderPageStepAdd);
}

void VolumeSlider::decreaseVolume()
{
    triggerAction(QAbstractSlider::SliderPageStepSub);
}

void VolumeSlider::generatePixmaps()
{
    pixmaps[0]=generatePixmap(false);
    pixmaps[1]=generatePixmap(true);
}

QPixmap VolumeSlider::generatePixmap(bool filled)
{
    bool reverse=isRightToLeft();
    QPixmap pix(size());
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setPen(textCol);
    for (int i=0; i<10; ++i) {
        int barHeight=(lineWidth*constHeightStep)*(i+1);
        QRect r(reverse ? pix.width()-(widthStep+(i*lineWidth*widthStep*2))
                        : i*lineWidth*widthStep*2,
                pix.height()-(barHeight+1), (lineWidth*widthStep)-1, barHeight);
        if (filled) {
            p.fillRect(r.adjusted(1, 1, 0, 0), textCol);
        } else if (lineWidth>1) {
            p.drawRect(r);
            p.drawRect(r.adjusted(1, 1, -1, -1));
        } else {
            p.drawRect(r);
        }
    }
    return pix;
}
