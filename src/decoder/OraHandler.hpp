/*
	This file is part of qt5-ora-plugin.

	qt5-ora-plugin is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	qt5-ora-plugin is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with qt5-ora-plugin.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ORA_DECODER_HPP
#define ORA_DECODER_HPP

#include "../images/Rgba.hpp"

struct OraDecoder{
		static void alpha_replace( RgbaView output, ConstRgbaView image, int dx, int dy );
		static void src_over(      RgbaView output, ConstRgbaView image, int dx, int dy );
};

class CgImage loadCgImage( class QIODevice& device );


#endif
