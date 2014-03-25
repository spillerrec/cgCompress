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

#include <utility>

#include <QList>
#include <QByteArray>

/** Describes a way to get from one image to another, and the file size cost.
 *  \todo support multiple steps in the conversion
 */
class Converter {
	public:
		struct Step{
			int from, to;
			Step( int from, int to ) : from(from), to(to) { }
			
			Image simple( const QList<Image>& base_images ) const{
				if( from != to )
					return base_images[from].difference( base_images[to] );
				else
					return base_images[from];
			}
			
			bool operator==( const Step& other ) const{ return from == other.from && to == other.to; }
		};
	private:
		QList<Image> base_images;
		QList<Step> steps;
		int size;
		
	public:
		/** \param [in] base_images The images to convert on
		 *  \param [in] from Index to the image to start on
		 *  \param [in] to Index to the image to end on
		 *  \param [in] format The format used for compressing
		 */
		Converter( QList<Image> base_images, QList<Step> steps, Format format )
			:	base_images(base_images)
			,	steps(steps) {
				size = get_primitive().remove_transparent().auto_crop().compressed_size( format, Format::MEDIUM );
			}
		
		const QList<Step>& getSteps() const{ return steps; }
		/** \return File size needed to store the conversion */
		int get_size() const{ return size; }
		
		/** \return The image used for converting **/
		Image get_primitive() const{
			if( steps.size() <= 0 )
				return Image{ {0,0}, QImage() };
			
			if( steps.size() == 1 )
				return steps[0].simple( base_images );
			else{
				Image temp = steps[0].simple( base_images );
				for( int i=1; i<steps.size(); i++ )
					temp = temp.contain_both( steps[i].simple( base_images ) );
				return temp;
			}
		}
		
		bool operator==( const Converter& other ) const{
			return steps == other.steps && base_images == other.base_images;
			//TODO: base_images comparison might be inefficient!
		}
};

#endif

