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
#include "SubImage.hpp"
#include "Compression.hpp"


static int compressed_lz4_size( const std::vector<uint8_t>& data ){
	return FileSize::lz4compress_size( data.data(), data.size() );
}


/** Calculates the sum of the absolute difference between touching pixels
 *  
 *  \param [in] img Image to calculate for
 *  \return The computed value
 */
int FileSize::image_gradient_sum( ConstRgbaView img ){
	int diffs = 0;
	
	for( auto row : img )
		for( int ix=1; ix<img.width(); ix++ ){
			//we add the difference in shown pixels
			Rgba left = row[ix-1], right = row[ix];
			diffs += abs( left.a - right.a );
			if( ( left.a > 0 ) && ( right.a > 0 ) ){
				diffs += abs( left.r - right.r );
				diffs += abs( left.g - right.g );
				diffs += abs( left.b - right.b );
			}
		}
	
	return diffs;
}


int FileSize::lz4compress_size( ConstRgbaView img ){
	std::vector<uint8_t> data;
	data.resize( img.width() * img.height() * 4 );
	
	//Encode data
	for( int iy=0; iy<img.height(); iy++ ){
		auto row = img[iy];
		for( int ix=0; ix<img.width(); ix++ ){
			auto pos = iy*img.width()*4 + ix*4;
			data[ pos + 0 ] = row[ix].r;
			data[ pos + 1 ] = row[ix].g;
			data[ pos + 2 ] = row[ix].b;
			data[ pos + 3 ] = row[ix].a;
		}
	}
	
	//Compress
	auto size = compressed_lz4_size( data );
	return size;
}
