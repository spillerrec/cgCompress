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
#include "images/blend.hpp"

#include <cmath>
#include <utility>

static uint8_t distance( uint8_t top, uint8_t bottom ){
	return std::abs( int(top) - bottom );
}

void ImageDiff( ConstRgbaView img1, ConstRgbaView img2, int max_diff ){
	//TODO: assert same size
	RgbaImage diff(    img1.width(), img1.height() );
	RgbaImage overlay( img1.width(), img1.height() );
	diff.fill( {} );
	overlay.fill( {} );
	
	for( int iy=0; iy<img1.height(); iy++ ){
		auto row1 = img1[iy];
		auto row2 = img2[iy];
		auto rowD = diff[iy];
		auto rowO = diff[iy];
		for( int ix=0; ix<img1.width(); ix++ ){
			auto dist = row1[ix].apply( row2[ix], &distance );
			
			if( std::max( dist.r, std::max( dist.g, dist.b ) ) > max_diff ){
				rowO[ix] = row1[ix];
				auto offset = Blending::diff_offset;
				rowD[ix] = { offset, offset, offset, 255 };
			}
			else{
				rowO[ix] = { 0,0,0,0 };
				rowD[ix] = row1[ix].apply( row2[ix], &Blending::getOffset );
			}
		}
	}
	
	Format f( "webp" );
	f.save( diff   , "ImageDiff-diff"    );
	f.save( overlay, "ImageDiff-overlay" );
}


void ImageDiffCombine( RgbaView base, ConstRgbaView diff, ConstRgbaView overlay ){
	Blending::BlendImages( base, diff, &Blending::applyOffsetRgba );
	Blending::BlendImages( base, diff, &Blending::srcOverDirty );
}

