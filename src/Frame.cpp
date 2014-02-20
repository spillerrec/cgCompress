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

#include "Frame.hpp"

/** \return The image this frame represent */
Image Frame::reconstruct() const{
	if( layers.size() == 0 )
		return Image( QPoint(0,0), QImage() );
	
	Image image( primitives[layers[0]] );
	for( int i=1; i<layers.size(); i++ )
		image = image.combine( primitives[layers[i]] );
	return image;
}


