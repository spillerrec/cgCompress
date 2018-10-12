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
class Image2{
	private:
		std::unique_ptr<T[]> data;
		int width { 0 };
		int height{ 0 };
		
	public:
		Image2() : data(nullptr), width(0), height(0) {}
		Image2( int width, int height )
			: data(std::make_unique<T[]>(width*height))
			, width(width), height(height) {}
			
		Image2( T* data, int width, int height )
			: data(data), width(width), height(height) {}
		
		
		operator ConstImageView<T>() const
			{ return {data.get(), width, height, width}; }
		operator      ImageView<T>() const
			{ return {data.get(), width, height, width}; }
};

#endif

