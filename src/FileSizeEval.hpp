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

#ifndef FILE_SIZE_EVAL_HPP
#define FILE_SIZE_EVAL_HPP

#include "images/Rgba.hpp"

/**
	Several methods to guess how much space a image will take to store compressed.
	However they are not compariable with final filesize or another metric
*/

class SubImage;
class QImage;

namespace FileSize{

int image_gradient_sum( ConstRgbaView img );
int image_gradient_sum( const SubImage& img, QImage mask, int pixel_different );
int lz4compress_size( ConstRgbaView img );

}

#endif

