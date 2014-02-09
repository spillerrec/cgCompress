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

class Converter {
	private:
		const QList<Image>& base_images;
		int from;
		int to;
		Image primitive;
		QByteArray data;
		
	public:
		Converter( const QList<Image>& base_images, int from, int to )
			:	base_images(base_images)
			,	from(from), to(to)
			,	primitive( base_images[from].difference( base_images[to] ) ) {
				data = primitive.remove_transparent().auto_crop().to_byte_array( "webp" );
			}
			
			int get_from() const{ return from; }
			int get_to() const{ return to; }
			QByteArray get_data() const{ return data; }
			Image get_primitive() const{ return primitive; }
		
};

#endif

