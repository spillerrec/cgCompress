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

#ifndef IMAGE_BLEND_HPP
#define IMAGE_BLEND_HPP

#include "Image.hpp"

namespace Blending{
	

inline uint8_t srcOverRgba( Rgba bottom, Rgba top ){
	uint8_t srcA = top.a;
	uint8_t dstA = mul8U( bottom.a, 255-srcA );
	uint8_t outA = srcA + dstA;
	
	auto over = [=]( int b, int t ){
		return (t * srcA + b * dstA) / outA;
	};
	
	Rgba out;
	out.a = outA;
	out.r = over( bottom.r, bottom.a, top.r, top.a );
	out.g = over( bottom.g, bottom.a, top.g, top.a );
	out.b = over( bottom.b, bottom.a, top.b, top.a );
	return out;
}

//Pick the top one if any alpha
inline uint8_t srcOverDirty( Rgba bottom, Rgba top ){
	return ( top.a != 0 ) ? top : bottom;
}

//Pick the top one if any alpha
inline uint8_t srcOverDirty( Rgba bottom, Rgba top ){
	return ( top != Rgba(255,0,255,0) ) ? top : bottom;
}

template<typename T, typename Func>
void BlendImages( ImageView<T> bottom, ConstImageView<T> overlay, Func blend ){
	//TODO: assert same size
	for( int iy=0; iy<overlay.width(); iy++ ){
		auto row_bottom  = bottom [iy];
		auto row_overlay = overlay[iy];
		for( int ix=0; ix<overlay.height(); ix++ )
			row_bottom[ix] = blend( row_bottom[ix], row_overlay[ix] );
	}
}

}

#endif

