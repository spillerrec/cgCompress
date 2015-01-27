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

#ifndef MULTI_IMAGE_HPP
#define MULTI_IMAGE_HPP

#include "Format.hpp"
#include "Image.hpp"
#include "Frame.hpp"

#include <utility>

/** Contains an image which is made up of many similar images */
class MultiImage {
	private:
		Format format;
		QList<Image> originals;
		
	public:
		/** Construct with images initialized
		 *  \param [in] format The image format to use
		 *  \param [in] originals The images which it is made of
		 */
		MultiImage( Format format, QList<Image> originals=QList<Image>() )
			: format(format), originals(originals){ }
		
		/** \param [in] original Another image that it is made of */
		void append( Image original ){ originals.append( original ); }
		
		bool optimize( QString name ) const;
		
		bool validate( QString file ) const;
};

#endif

