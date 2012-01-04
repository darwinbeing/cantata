/*
 * Cantata
 *
 * Copyright (c) 2011-2012 Craig Drummond <craig.p.drummond@gmail.com>
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

#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#ifdef ENABLE_KDE_SUPPORT
#include <KDialog>
#else
#include <QDialog>
#endif

class QTimer;
class QLabel;

#ifdef ENABLE_KDE_SUPPORT
class UpdateDialog : public KDialog
#else
class UpdateDialog : public QDialog
#endif
{
    Q_OBJECT

public:
    UpdateDialog(QWidget *parent);

public Q_SLOTS:
    void show();
    void hide();
    void display();
    void complete();

private:
    QTimer *timer;
    bool done;
    QLabel *label;
};

#endif
