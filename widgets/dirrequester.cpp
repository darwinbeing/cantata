/*
 * Cantata
 *
 * Copyright 2011 Craig Drummond <craig.p.drummond@gmail.com>
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

#include "dirrequester.h"
#include <QtGui/QToolButton>
#include <QtGui/QFileDialog>
#include <QtGui/QHBoxLayout>
#include <QtGui/QIcon>

DirRequester::DirRequester(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *layout=new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    edit=new LineEdit(this);
    QToolButton *button=new QToolButton(this);
    layout->addWidget(edit);
    layout->addWidget(button);
    button->setAutoRaise(true);
    button->setIcon(QIcon::fromTheme("document-open"));
    connect(button, SIGNAL(clicked(bool)), SLOT(chooseDir()));
}

void DirRequester::chooseDir()
{
    QString dir=QFileDialog::getExistingDirectory(this, tr("Select Folder"));
    if (!dir.isEmpty()) {
        edit->setText(dir);
    }
}
