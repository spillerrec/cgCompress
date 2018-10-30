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

#ifndef IMAGE_SIMILARITIES_HPP
#define IMAGE_SIMILARITIES_HPP

#include <QImage>
#include <QList>
#include <memory>
#include <vector>

class Image;

class ImageSimMask{
	private:
		QImage mask;
		
	public:
		ImageSimMask( int width, int height );
		ImageSimMask( QSize size ) : ImageSimMask( size.width(), size.height() ) { }
		
		auto width() const { return mask.width(); }
		auto height() const { return mask.height(); }
		auto size() const { return mask.size(); }
		auto scanLine( int iy ){ return mask.scanLine( iy ); }
		auto constScanLine( int iy ) const { return mask.constScanLine( iy ); }
		void fill( unsigned value ){ mask.fill( value ); }
		
		void combineMasks( ImageSimMask combine_with );
		Image apply( QImage image ) const;
};

class RefImage{
	private:
		std::unique_ptr<uint16_t[]> refs;
		int width;
		int height;
		
	public:
		RefImage( int width, int height );
		
		uint16_t* getRow( int iy );
		
		void setMaskTo( ImageSimMask mask, uint16_t value );
		void fill( uint16_t value );
		ImageSimMask getMaskOf( uint16_t value );
};

/** Contains an image which is made up of many similar images */
class ImageSimilarities {
	private:
		std::vector<QImage> originals;
		std::vector<RefImage> refs;
		
		
		
	public:
		void addImage( QImage img );
		Image getImagePart( int id, int ref );
		ImageSimMask getMask( int id, int ref );
};

#endif

