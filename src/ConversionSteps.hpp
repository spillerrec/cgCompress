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

#ifndef CONVERSION_STEPS_HPP
#define CONVERSION_STEPS_HPP


#include "Conversion.hpp"

class ConversionSteps {
	private:
		QList<Image> base_images;
		QList<Converter> converters;
		QList<Conversion> conversions;
		QList<int> has; //Finished frames
		int amount;
		
		Conversion convertionTo( int image ) const;
		QList<Conversion> optimizedConverters( QList<Conversion> in ) const;
		
	public:
		ConversionSteps( QList<Image> base_images, QList<Converter> converters )
			:	base_images(base_images), converters(converters), amount(base_images.size()) { }
		
		QList<Conversion> usableConvertions() const;
		
		QList<Conversion> getConversionsTo( int image ) const;
		
		void addBaseImage( int base );
		QList<Conversion> getConversions() const{ return conversions; }
		
		void addConvertion( Conversion c );
		
		void findBestConverters();
		
		int fileSize() const;
};

#endif

