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

#ifndef IMAGE_OPTIM_HPP
#define IMAGE_OPTIM_HPP

#include "images/Rgba.hpp"

void ImageDiffAdd( RgbaView base, ConstRgbaView diff );

void ImageDiff( ConstRgbaView img1, ConstRgbaView img2, int diff_amount );
void ImageDiffCombine( RgbaView base, ConstRgbaView diff, ConstRgbaView overlay );


#endif

