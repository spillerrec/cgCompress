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

#ifndef IMAGE_RGBA_HPP
#define IMAGE_RGBA_HPP

#include "Image.hpp"

struct Rgba{
	uint8_t b{ 0 };
	uint8_t g{ 0 };
	uint8_t r{ 0 };
	uint8_t a{ 0 };
	
	Rgba() {};
	Rgba( uint8_t r, uint8_t g, uint8_t b, uint8_t a )
		: b(b), g(g), r(r), a(a) { }
		
	bool operator!=( Rgba other )
		{ return r!=other.r || g!=other.g || b!=other.b || a!=other.a; }
};

inline uint8_t mul8U( uint8_t left, uint8_t right ){
	return (int(left) * int(right)) >> 8;
}

using RgbaImage = Image2<Rgba>;
using RgbaView = ImageView<Rgba>;
using ConstRgbaView = ConstImageView<Rgba>;

class QImage toQImage( ConstRgbaView image );
RgbaImage fromQImage( class QImage in );

void removeAlpha( RgbaView image );
void removeTransparent( RgbaView image, Rgba clear_color = {0,0,0,0} );

#endif

