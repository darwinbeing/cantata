/*
 * Cantata
 *
 * Copyright 2011 Craig Drummond <craig.p.drummond@gmail.com>
 *
 */
/*
 * Copyright (c) 2008 Sander Knopper (sander AT knopper DOT tk) and
 *                    Roeland Douma (roeland AT rullzer DOT com)
 *
 * This file is part of QtMPC.
 *
 * QtMPC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * QtMPC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QtMPC.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MPD_PARSE_UTILS_H
#define MPD_PARSE_UTILS_H

#include <QString>

class Song;
class Playlist;
class DirViewItemRoot;
class MusicLibraryItemRoot;
class Output;

class MPDParseUtils
{
public:
    static QString getDir(const QString &f);
    static QList<Playlist> parsePlaylists(const QByteArray &data);
    static void parseStats(const QByteArray &data);
    static void parseStatus(const QByteArray &data);
    static Song parseSong(const QByteArray &data);
    static QList<Song> parseSongs(const QByteArray &data);
    static MusicLibraryItemRoot * parseLibraryItems(const QByteArray &data);
    static DirViewItemRoot * parseDirViewItems(const QByteArray &data);
    static QString seconds2formattedString(const quint32 totalseconds);
    static QList<Output> parseOuputs(const QByteArray &data);
};

#endif
