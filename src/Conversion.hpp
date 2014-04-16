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

#ifndef CONVERSION_HPP
#define CONVERSION_HPP

#include "Converter.hpp"

/** Describes a way to get from one image to another, and the file size cost.
 */
class Conversion {
		Converter const* converter;
	public:
		int from;
		int to;
		int size;
		
		Conversion( const Converter& converter, int from, int to, int size )
			:	converter(&converter), from(from), to(to), size(size) { }
		
		Converter const* getConverter() const{ return converter; }
		
		bool operator==( const Conversion& other ) const{
			return from == other.from && to == other.to && converter == other.converter;
		}
		
		bool operator<( const Conversion& other ) const{
			return size < other.size;
		}
};

#endif

