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

class MultiImage {
	private:
		QList<Image> originals;
		
		QList<Image> diff_fast( int& amount ) const;
		QList<Image> diff_linear( int& amount ) const;
		QList<Image> diff_all( int& amount ) const;
		
		static std::pair<QList<Frame>,int> lowest_cost( const QList<int>& costs, QList<QList<Frame>> all_frames, QList<int> used=QList<int>() );
		
		Format format;
		
	public:
		MultiImage() { }
		MultiImage( QList<Image> originals ) : originals(originals) { }
		
		void append( Image original ){ originals.append( original ); }
		void set_format( Format format ){ this->format = format; }
		
		QList<Frame> optimize( QString name ) const;
		QList<Frame> optimize2( QString name ) const;
};

#endif

