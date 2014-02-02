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

#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <QImage>

class Image {
	private:
		QPoint pos;
		QImage img;
		
	public:
		Image( QString path ) : img( QImage(path).convertToFormat(QImage::Format_ARGB32) ) { }
		Image( QPoint pos, QImage img ) : pos(pos), img(img) { }
		
		bool save( QString path ){ return img.save( path ); }
		
		Image sub_image( int x, int y, int width, int height ) const{
			return Image( pos+QPoint(x,y), img.copy( x,y, width,height ) );
		}
		
		QList<Image> segment() const;
		
		unsigned compressed_size() const;
		Image difference( Image img ) const;
		Image remove_transparent() const;
		Image auto_crop() const;
		
		bool operator==( const Image& other ) const{
			return pos == other.pos && img == other.img;
		}
};

#endif

