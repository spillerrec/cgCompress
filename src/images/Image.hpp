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

#ifndef IMAGE2_HPP
#define IMAGE2_HPP

#include "ImageView.hpp"
#include <memory>

template<typename T>
class Image2 : public ImageView<T>{
	private:
		std::unique_ptr<T[]> data;
		
	public:
		Image2( T* data_ptr, int width, int height )
			: ImageView<T>(data_ptr, width, height), data(data_ptr) {}
			
		Image2() : Image2(nullptr, 0, 0) {}
		Image2( int width, int height )
			: Image2( new T[width*height], width, height ) {}
};

#endif

