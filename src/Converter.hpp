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

#ifndef CONVERTER_HPP
#define CONVERTER_HPP

#include "Image.hpp"

#include <QList>
#include <utility>

#include <QByteArray>
#include <vector>

/** Describes a way to get from one image to another, and the file size cost.
 *  \todo support multiple steps in the conversion
 */
class Converter {
	private:
		const QList<Image>* base_images{ nullptr };
		int from;
		int to;
		int size;
		
	public:
		Converter(){} //NOTE: only for QtConcurrent
		/** \param [in] base_images The images to convert on
		 *  \param [in] from Index to the image to start on
		 *  \param [in] to Index to the image to end on
		 *  \param [in] format The format used for compressing
		 */
		Converter( const QList<Image>& base_images, int from, int to, Format format )
			:	base_images(&base_images)
			,	from(from), to(to) {
				size = get_primitive().auto_crop().compressed_size( format, Format::MEDIUM ); //TODO: fix format
			}
		
		/** \return Index to the start image */
		int get_from() const{ return from; }
		
		/** \return Index to the resulting image */
		int get_to() const{ return to; }
		
		/** \return File size needed to store the conversion */
		int get_size() const{ return size; }
		
		/** \return The image used for converting **/
		Image get_primitive() const;
		
		static std::vector<int> path( const std::vector<Converter>& converters, int from, int to=0 );
		
		static auto less_size( const Converter& a, const Converter& b ){ return a.size < b.size; }
		static auto less_to(   const Converter& a, const Converter& b ){ return a.to   < b.to  ; }
};

#endif

