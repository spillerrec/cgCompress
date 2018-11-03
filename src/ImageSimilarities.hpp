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

#include <memory>
#include <vector>

#include "images/Rgba.hpp"

class Image;

using ImageSimMask = Image2<bool>;
using RefImage     = Image2<uint16_t>;

void combineMasks( ImageView<bool> mask, ConstImageView<bool> other );
Image applyMask( ConstImageView<bool> mask, ConstRgbaView image );

void setRefTo( ImageView<uint16_t> ref, ConstImageView<bool> mask, uint16_t value );
ImageSimMask getMaskFrom( ConstImageView<uint16_t> refs, uint16_t vakye );


/** Contains an image which is made up of many similar images */
class ImageSimilarities {
	private:
		std::vector<ConstRgbaView> originals;
		std::vector<RefImage> refs;
		
		
		
	public:
		void addImage( ConstRgbaView img );
		Image getImagePart( int id, int ref );
		ImageSimMask getMask( int id, int ref );
};

#endif

