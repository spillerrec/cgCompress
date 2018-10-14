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

class QImage;
class QByteArray;
class QIODevice;

#include "../images/Rgba.hpp" //TODO: Avoid


namespace FormatWebP{
	QImage read( QByteArray data );
	RgbaImage readRgbaImage( QByteArray data );
	bool write( QImage image, QIODevice& device, bool keep_alpha=true, int quality=100 );
	bool write( ConstRgbaView image, QIODevice& device, bool keep_alpha=true, int quality=100 );
	bool writeLossy( QImage, QIODevice& device, int quality );
	int estimate_filesize( QImage image, bool keep_alpha );
}

