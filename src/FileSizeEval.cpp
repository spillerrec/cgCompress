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

#include "FileSizeEval.hpp"
#include "SubQImage.hpp"
#include "Compression.hpp"


static int compressed_lz4_size( const std::vector<uint8_t>& data ){
	return FileSize::lz4compress_size( data.data(), data.size() );
}


int FileSize::simple_alpha( QImage mask, int transparent ){
	//TODO:
	return 0;
}

/** Calculates the sum of the absolute difference between touching pixels
 *  
 *  \param [in] img Image to calculate for
 *  \return The computed value
 */
int FileSize::image_gradient_sum( QImage img ){
	int diffs = 0;
	
	auto w = img.width();
	for( int iy=0; iy<img.height(); iy++ ){
		auto row = (const QRgb*) img.constScanLine( iy );
		for( int ix=1; ix<w/*img.width()*/; ix++ ){
			//we add the difference in shown pixels
			QRgb left = row[ix-1], right = row[ix];
			diffs += abs( qAlpha(left) - qAlpha(right) );
			if( ( qAlpha(left) > 0 ) && ( qAlpha(right) > 0 ) ){
				diffs += abs( qRed(left) - qRed(right) );
				diffs += abs( qGreen(left) - qGreen(right) );
				diffs += abs( qBlue(left) - qBlue(right) );
			}
		}
	}
	
	return diffs;
}

int FileSize::image_gradient_sum( const SubQImage& img, QImage mask, int pixel_different ){
	int diffs = 0;
	
	auto w = img.width();
	for( int iy=0; iy<img.height(); iy++ ){
		auto row_alpha = mask.constScanLine( iy );
		auto row = img.row( iy );
		for( int ix=1; ix<w; ix++ ){
			auto alpha_left  = row_alpha[ix-1] != pixel_different;
			auto alpha_right = row_alpha[ix  ] != pixel_different;
			diffs += (alpha_left != alpha_right) ? 255 : 0;
			//TODO: not great for images with transparency
			//we add the difference in shown pixels
			if( !(alpha_left || alpha_right) ){
				QRgb left = row[ix-1], right = row[ix];
				diffs += abs( qRed(  left) - qRed(  right) );
				diffs += abs( qGreen(left) - qGreen(right) );
				diffs += abs( qBlue( left) - qBlue( right) );
			}
		}
	}
	
	return diffs;
}

int FileSize::lz4compress_size( QImage img ){
	std::vector<uint8_t> data;
	data.resize( img.width() * img.height() * 4 );
	
	//Encode data
	for( int iy=0; iy<img.height(); iy++ ){
		auto row = (const QRgb*) img.constScanLine( iy );
		for( int ix=0; ix<img.width(); ix++ ){
			auto pos = iy*img.width()*4 + ix*4;
			data[ pos + 0 ] = qRed(   row[ix] );
			data[ pos + 1 ] = qGreen( row[ix] );
			data[ pos + 2 ] = qBlue(  row[ix] );
			data[ pos + 3 ] = qAlpha( row[ix] );
		}
	}
	
	//Compress
	auto size = compressed_lz4_size( data );
	return size;
}
