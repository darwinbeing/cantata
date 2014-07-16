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

#include "albumspage.h"
#include "mpd/mpdconnection.h"
#include "covers.h"
#include "models/musiclibrarymodel.h"
#include "models/musiclibraryitemsong.h"
#include "models/albumsmodel.h"
#include "support/localize.h"
#include "support/messagebox.h"
#include "settings.h"
#include "stdactions.h"
#include "support/utils.h"

AlbumsPage::AlbumsPage(QWidget *p)
    : QWidget(p)
{
    setupUi(this);
    addToPlayQueue->setDefaultAction(StdActions::self()->addToPlayQueueAction);
    replacePlayQueue->setDefaultAction(StdActions::self()->replacePlayQueueAction);

    view->addAction(StdActions::self()->addToPlayQueueAction);
    view->addAction(StdActions::self()->addRandomToPlayQueueAction);
    view->addAction(StdActions::self()->replacePlayQueueAction);
    view->addAction(StdActions::self()->addWithPriorityAction);
    view->addAction(StdActions::self()->addToStoredPlaylistAction);
    #ifdef TAGLIB_FOUND
    #ifdef ENABLE_DEVICES_SUPPORT
    view->addAction(StdActions::self()->copyToDeviceAction);
    #endif
    view->addAction(StdActions::self()->organiseFilesAction);
    view->addAction(StdActions::self()->editTagsAction);
    #ifdef ENABLE_REPLAYGAIN_SUPPORT
    view->addAction(StdActions::self()->replaygainAction);
    #endif
    view->addAction(StdActions::self()->setCoverAction);
    #ifdef ENABLE_DEVICES_SUPPORT
    QAction *sep=new QAction(this);
    sep->setSeparator(true);
    view->addAction(sep);
    view->addAction(StdActions::self()->deleteSongsAction);
    #endif
    #endif // TAGLIB_FOUND

    proxy.setSourceModel(AlbumsModel::self());
    view->setModel(&proxy);

    connect(MusicLibraryModel::self(), SIGNAL(updateGenres(const QSet<QString> &)), genreCombo, SLOT(update(const QSet<QString> &)));
    connect(this, SIGNAL(add(const QStringList &, bool, quint8)), MPDConnection::self(), SLOT(add(const QStringList &, bool, quint8)));
    connect(this, SIGNAL(addSongsToPlaylist(const QString &, const QStringList &)), MPDConnection::self(), SLOT(addToPlaylist(const QString &, const QStringList &)));
    connect(genreCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(searchItems()));
    connect(view, SIGNAL(searchItems()), this, SLOT(searchItems()));
    connect(view, SIGNAL(itemsSelected(bool)), this, SLOT(controlActions()));
    connect(view, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(itemDoubleClicked(const QModelIndex &)));
    connect(view, SIGNAL(rootIndexSet(QModelIndex)), this, SLOT(updateGenres(QModelIndex)));
    connect(MPDConnection::self(), SIGNAL(updatingLibrary()), this, SLOT(showSpinner()));
    connect(MPDConnection::self(), SIGNAL(updatedLibrary()), this, SLOT(hideSpinner()));
    connect(MPDConnection::self(), SIGNAL(updatingDatabase()), this, SLOT(showSpinner()));
    connect(MPDConnection::self(), SIGNAL(updatedDatabase()), this, SLOT(hideSpinner()));
    view->load(metaObject()->className());
}

AlbumsPage::~AlbumsPage()
{
    view->save(metaObject()->className());
}

void AlbumsPage::showEvent(QShowEvent *e)
{
    view->focusView();
    QWidget::showEvent(e);
}

void AlbumsPage::clear()
{
    AlbumsModel::self()->clear();
    view->update();
}

QStringList AlbumsPage::selectedFiles(bool allowPlaylists, bool randomAlbums) const
{
    QModelIndexList selected = view->selectedIndexes();
    if (selected.isEmpty()) {
        return QStringList();
    }
    bool filteredOnly=proxy.enabled() && Settings::self()->filteredOnly();
    QModelIndexList mapped=proxy.mapToSource(selected, filteredOnly);
    if (randomAlbums) {
        if (filteredOnly) {
            QMap<quint32, QModelIndexList> albums;
            foreach (const QModelIndex &idx, mapped) {
                if (idx.parent().isValid()) {
                    albums[idx.parent().row()].append(idx);
                }
            }
            QList<quint32> keys=albums.keys();
            if (keys.isEmpty()) {
                return QStringList();
            } else if (1==keys.count()) {
                mapped=albums.begin().value();
            } else {
                mapped.clear();
                while (!keys.isEmpty()) {
                    mapped.append(albums[keys.takeAt(Utils::random(keys.count()))]);
                }
            }
        } else {
            QModelIndexList albumIndexes;
            foreach (const QModelIndex &idx, mapped) {
                if (static_cast<AlbumsModel::Item *>(idx.internalPointer())->isAlbum()) {
                    albumIndexes.append(idx);
                }
            }

            if (albumIndexes.isEmpty()) {
                return QStringList();
            }

            if (1==albumIndexes.count()) {
                mapped=albumIndexes;
            } else {
                mapped.clear();
                while (!albumIndexes.isEmpty()) {
                    mapped.append(albumIndexes.takeAt(Utils::random(albumIndexes.count())));
                }
            }
        }
    }
    return AlbumsModel::self()->filenames(mapped, allowPlaylists);
}

QList<Song> AlbumsPage::selectedSongs(bool allowPlaylists) const
{
    QModelIndexList selected = view->selectedIndexes();
    if (selected.isEmpty()) {
        return QList<Song>();
    }
    return AlbumsModel::self()->songs(proxy.mapToSource(selected, proxy.enabled() && Settings::self()->filteredOnly()), allowPlaylists);
}

Song AlbumsPage::coverRequest() const
{
    QModelIndexList selected = view->selectedIndexes(false); // Dont need sorted selection here...

    if (1==selected.count()) {
        QList<Song> songs=AlbumsModel::self()->songs(QModelIndexList() << proxy.mapToSource(selected.at(0)), false);
        if (!songs.isEmpty()) {
            return songs.at(0);
        }
    }
    return Song();
}

void AlbumsPage::addSelectionToPlaylist(const QString &name, bool replace, quint8 priorty, bool randomAlbums)
{
    QStringList files=selectedFiles(name.isEmpty(), randomAlbums);

    if (!files.isEmpty()) {
        if (name.isEmpty()) {
            emit add(files, replace, priorty);
        } else {
            emit addSongsToPlaylist(name, files);
        }
        view->clearSelection();
    }
}

#ifdef ENABLE_DEVICES_SUPPORT
void AlbumsPage::addSelectionToDevice(const QString &udi)
{
    QList<Song> songs=selectedSongs();

    if (!songs.isEmpty()) {
        emit addToDevice(QString(), udi, songs);
        view->clearSelection();
    }
}

void AlbumsPage::deleteSongs()
{
    QList<Song> songs=selectedSongs();

    if (!songs.isEmpty()) {
        if (MessageBox::Yes==MessageBox::warningYesNo(this, i18n("Are you sure you wish to delete the selected songs?\nThis cannot be undone."),
                                                      i18n("Delete Songs"), StdGuiItem::del(), StdGuiItem::cancel())) {
            emit deleteSongs(QString(), songs);
        }
        view->clearSelection();
    }
}
#endif

void AlbumsPage::itemDoubleClicked(const QModelIndex &)
{
    if (1==view->selectedIndexes(false).size()) {//doubleclick should only have one selected item
        addSelectionToPlaylist();
    }
}

void AlbumsPage::searchItems()
{
    QString text=view->searchText().trimmed();
    proxy.update(text, genreCombo->currentIndex()<=0 ? QString() : genreCombo->currentText());
    if (proxy.enabled() && !proxy.filterText().isEmpty()) {
        view->expandAll();
    }
}

void AlbumsPage::updateGenres(const QModelIndex &idx)
{
    if (idx.isValid()) {
        QModelIndex m=proxy.mapToSource(idx);
        if (m.isValid() && static_cast<AlbumsModel::Item *>(m.internalPointer())->isAlbum()) {
            genreCombo->update(static_cast<AlbumsModel::AlbumItem *>(m.internalPointer())->genres);
            return;
        }
    }
    genreCombo->update(MusicLibraryModel::self()->genres());
}

void AlbumsPage::controlActions()
{
    QModelIndexList selected=view->selectedIndexes(false); // Dont need sorted selection here...
    bool enable=selected.count()>0;

    StdActions::self()->addToPlayQueueAction->setEnabled(enable);
    StdActions::self()->addWithPriorityAction->setEnabled(enable);
    StdActions::self()->replacePlayQueueAction->setEnabled(enable);
    StdActions::self()->addToStoredPlaylistAction->setEnabled(enable);
    #ifdef TAGLIB_FOUND
    StdActions::self()->organiseFilesAction->setEnabled(enable && MPDConnection::self()->getDetails().dirReadable);
    StdActions::self()->editTagsAction->setEnabled(StdActions::self()->organiseFilesAction->isEnabled());
    #ifdef ENABLE_REPLAYGAIN_SUPPORT
    StdActions::self()->replaygainAction->setEnabled(StdActions::self()->organiseFilesAction->isEnabled());
    #endif
    #ifdef ENABLE_DEVICES_SUPPORT
    StdActions::self()->deleteSongsAction->setEnabled(StdActions::self()->organiseFilesAction->isEnabled());
    StdActions::self()->copyToDeviceAction->setEnabled(StdActions::self()->organiseFilesAction->isEnabled());
    #endif
    #endif // TAGLIB_FOUND
    StdActions::self()->setCoverAction->setEnabled(1==selected.count() && static_cast<AlbumsModel::Item *>(proxy.mapToSource(selected.at(0)).internalPointer())->isAlbum());

    bool allowRandomAlbum=!selected.isEmpty();
    if (allowRandomAlbum) {
        foreach (const QModelIndex &idx, selected) {
            if (!static_cast<AlbumsModel::Item *>(proxy.mapToSource(idx).internalPointer())->isAlbum()) {
                allowRandomAlbum=false;
                break;
            }
        }
    }
    StdActions::self()->addRandomToPlayQueueAction->setEnabled(allowRandomAlbum);
}

void AlbumsPage::showSpinner()
{
    view->showSpinner();
    view->showMessage(i18n("Updating..."), -1);
}

void AlbumsPage::hideSpinner()
{
    view->hideSpinner();
    view->showMessage(QString(), 0);
}
