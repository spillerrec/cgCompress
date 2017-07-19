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

#ifndef SUB_QIMAGE_HPP
#define SUB_QIMAGE_HPP

#include <QImage>


/** Provides ReadOnly access to a region of a QImage without copying.
 *  row() and rowIndex() provides pixel access which is offset and casted correctly */
class SubQImage{
	private:
		QImage img;
		QPoint pos;
		QSize subsize;
		
		auto scanLine( int iy ) const
			{ return img.constScanLine( iy + pos.y() ); }
		
		SubQImage( QImage img, QPoint pos, QSize subsize )
			:	img(img), pos(pos), subsize(subsize) {}
	public:
		SubQImage( QImage img, QPoint pos={0,0} )
			:	img(img), pos(pos), subsize(img.size()) { }
		
		auto offset() const{ return pos; }
		
		auto size()   const{ return subsize; }
		auto width()  const{ return size().width();  }
		auto height() const{ return size().height(); }
		
		auto rowIndex( int iy ) const{ return               scanLine( iy )  + pos.x(); }
		auto row(      int iy ) const{ return (const QRgb*)(scanLine( iy )) + pos.x(); }
		
		auto get() const
			{ return img.copy( pos.x(), pos.y(), width(), height() ); }
		
		SubQImage copy( QPoint pos, QSize size ) const
			{ return { img, offset() + pos, size }; }
		
		bool operator==( const SubQImage& other ) const
			{	return img == other.img && pos == other.pos && subsize == other.subsize; }
};

#endif

