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

#include "Format.hpp"
#include "FileSizeEval.hpp"

#include <QBuffer>


/** Compress image to a memory buffer
 *  
 *  \param [in] img Image to save
 *  \return buffer containing the compressed image
 */
QByteArray Format::to_byte_array( QImage img ) const{
	QByteArray data;
	QBuffer buffer( &data );
	buffer.open( QIODevice::WriteOnly );
	img.save( &buffer, ext(), get_quality() );
	return data;
}

/** Estimate file size when compressed
 *  
 *  \param [in] img Image to estimate
 *  \param [in] p Precision of the estimation
 *  \return The estimated file size
 */
int Format::file_size( QImage img, Precision p ) const{
	if( precision_level > 0 && p != HIGH )
		return FileSize::image_gradient_sum( img );
	return to_byte_array( img ).size();
}

