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

#include "ImageOptim.hpp"
#include "Format.hpp"

#include <QImage>
#include <cmath>
#include <utility>

uint8_t distance( uint8_t top, uint8_t bottom ){
	return std::abs( int(top) - bottom );
}

const uint8_t offset = 127;
uint8_t getOffset( uint8_t pixel_top, uint8_t pixel_bottom ){
	return pixel_top - pixel_bottom + offset;
}
uint8_t applyOffset( uint8_t base, uint8_t diff ){
	return base + diff - offset;
}

void ImageDiff( QImage img1, QImage img2, int max_diff ){
	QImage diff(    img1.size(), QImage::Format_ARGB32 );
	QImage overlay( img1.size(), QImage::Format_ARGB32 );
	
	diff.fill( 0 );
	overlay.fill( 0 );
	
	for( int iy=0; iy<img1.height(); iy++ ){
		for( int ix=0; ix<img1.width(); ix++ ){
			QPoint pos( ix, iy );
			auto a = img1.pixel( pos );
			auto b = img2.pixel( pos );
			
			auto dist_r = distance( qRed(   a ), qRed(   b ) );
			auto dist_g = distance( qGreen( a ), qGreen( b ) );
			auto dist_b = distance( qBlue(  a ), qBlue(  b ) );
			
			if( std::max( dist_r, std::max( dist_g, dist_b ) ) > max_diff ){
				overlay.setPixel( pos, a );
				diff   .setPixel( pos, qRgb( offset, offset, offset ) );
			}
			else{
				overlay.setPixel( pos, qRgba( 0,0,0,0 ) );
				auto get = [&]( int (*func)(QRgb) ){ return getOffset( func( a ), func( b ) ); };
				diff   .setPixel( pos, qRgb( get( qRed ), get( qGreen ), get( qBlue ) ) );
			}
		}
	}
	
	Format f( "webp" );
	f.save( diff   , "ImageDiff-diff"    );
	f.save( overlay, "ImageDiff-overlay" );
}


void ImageDiffCombine( QImage base, QImage diff, QImage overlay ){
	//Apply diff
	for( int iy=0; iy<base.height(); iy++ ){
		for( int ix=0; ix<base.width(); ix++ ){
			QPoint pos( ix, iy );
			auto b = base.pixel( pos );
			auto d = diff.pixel( pos );
			
			auto get = [&]( int (*func)(QRgb) ){ return applyOffset( func( b ), func( d ) ); };
			base.setPixel( pos, qRgb( get( qRed ), get( qGreen ), get( qBlue ) ) );
		}
	}
	
	//Apply overlay
	for( int iy=0; iy<base.height(); iy++ ){
		for( int ix=0; ix<base.width(); ix++ ){
			QPoint pos( ix, iy );
			auto b = base.pixel( pos );
			auto o = overlay.pixel( pos );
			
			if( qAlpha( o ) > 0 )
				base.setPixel( pos, o );
		}
	}
	
	Format f( "webp" );
	f.save( base   , "ImageDiff-result"    );
}

