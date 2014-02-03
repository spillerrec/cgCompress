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

#ifndef FRAME_HPP
#define FRAME_HPP

#include "Image.hpp"

#include <QList>

class Frame {
	public:
		QList<Image> primitives;
		QList<int> layers;
		
		Frame( QList<Image> primitives ) : primitives( primitives ) { }
		
		static QList<Frame> generate_frames( QList<Image>& primitives, Image original, int start, Frame current );
		
	public:
		Image reconstruct() const;
		
		static QList<Frame> generate_frames( QList<Image>& primitives, Image original, int start );
};

#endif

