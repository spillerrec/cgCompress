/*
	This file is part of cgCompress.

	cgCompress is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cgCompress is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cgCompress.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef FILE_UTILS_HPP
#define FILE_UTILS_HPP

#include <QString>
#include <QDir>
#include <QImage>

#include "Format.hpp"

QList<std::pair<QString,QImage>> extract_files( QString filename );

void extract_cgcompress( QString filename, Format format );

void evaluate_cgcompress( QStringList files );

void pack_directory( QDir dir, QString name_extension );

bool isSimilar( QImage img1, QImage img2 );

QStringList expandFolders( QStringList files );

QImage discardTransparent( QImage img, QRgb discard_color = qRgb(0,0,0) );
QImage withoutAlpha( QImage img );

#endif

