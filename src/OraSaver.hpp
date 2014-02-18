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

#ifndef ORA_SAVER_HPP
#define ORA_SAVER_HPP

#include "Format.hpp"
#include "Image.hpp"
#include "Frame.hpp"

#include <utility>

/** Save images in a zip archive using the OpenRaster conventions.
 *  This means, STORED mimetype file as the first file, stack.xml file
 *  describing the image contents, a thumbnail in Thumbnails/thumbnail.*,
 *  and data files in data/.
 */
class OraSaver {
	private:
		QList<Image> primitives;
		QList<Frame> frames;
		
	public:
		OraSaver( QList<Image> primitives, QList<Frame> frames )
			:	primitives(primitives), frames(frames) { }
		OraSaver( QList<Image> images );
		
		void save( QString path, Format format ) const;
		
		static void save( QString path, QString mimetype, QString stack, QList<std::pair<QString,QByteArray>> files );
};

#endif

