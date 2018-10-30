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

#include <vector>

/** A single frame in a multi image */
class Frame {
	public:
		/// Images for reconstructing the original image
		std::vector<Image> primitives;
		
		/// The indexes to primitives used for reconstructing
		std::vector<int> layers;
		
		/** \param [in] primitives The primitives used to build the frame
		  * \param [in] layers The specific order of the primitives used */
		Frame( std::vector<Image> primitives, std::vector<int> layers )
			: primitives( std::move(primitives) ), layers( std::move(layers) ) { }
		
		Image reconstruct() const;
		
		void update_ids( int from, std::vector<int> to );
		
		void remove_pointless_layers();
};

#endif

