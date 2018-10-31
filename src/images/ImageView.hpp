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

#ifndef IMAGE_VIEW_HPP
#define IMAGE_VIEW_HPP

#include <cstring>
#include <algorithm>

template<typename T>
class RowIt{
	private:
		T* data;
		int size;
		int stride;
		
	public:
		RowIt( T* data, int size, int stride )
			:	data(data), size(size), stride(stride) {}
		
		auto begin() const{ return data; }
		auto end()   const{ return data+size; }
		
		RowIt<T>& operator++() {
			data += stride;
			return *this;
		}
		
		bool operator!=( const RowIt<T>& other ) const
			{ return data != other.data; }
			
		T& operator[]( int x ) const { return data[x]; }
			
		RowIt<T>& operator*() { return *this; }
		
};

template<typename T>
class ImageViewBase{
	protected:
		T* data;
		int w, h; //width, height
		int s; //stride
		
		T* fromOffset( int x, int y ) const
			{ return data + y*stride() + x; }
		
	public:
		ImageViewBase( T* data, int width, int height, int stride )
			:	data(data), w(width), h(height), s(stride) { }
			
		ImageViewBase( T* data, int width, int height )
			:	ImageViewBase<T>( data, width, height, width ) { }
		
		auto rawData() const{ return data; }
		auto width()   const{ return w; }
		auto height()  const{ return h; }
		auto stride()  const{ return s; }
		bool valid()   const{ return data && width()>0 && height()>0; }
		
		auto operator[]( int y ) const
			{ return RowIt<T>( fromOffset(0,y), width(), stride() ); }
			
		auto begin() const{ return (*this)[0]; }
		auto end()   const{ return (*this)[h]; }
		
		bool operator==( ImageViewBase<T> other ) const{
			if( w != other.w || h != other.h )
				return false;
			for( int iy=0; iy<h; iy++ )
				if( !std::equal( other[iy].begin(), other[iy].end(), (*this)[iy].begin() ) )
					return false;
			return true;
		}
};

template<typename T>
class ConstImageView : public ImageViewBase<const T>{
	public:
		ConstImageView() : ImageViewBase<const T>( nullptr, 0, 0, 0) { }
			
		ConstImageView( const T* data, int width, int height, int stride )
			:	ImageViewBase<const T>( data, width, height, stride) { }
			
		ConstImageView( const T* data, int width, int height )
			:	ConstImageView( data, width, height, width ) { }
			
		
		ConstImageView<T> crop( int x, int y, int newWidth, int newHeight) const
			{ return { this->fromOffset( x, y ), newWidth, newHeight, this->stride() }; }
};

template<typename T>
class ImageView : public ImageViewBase<T>{
	public:
		ImageView() : ImageViewBase<const T>( nullptr, 0, 0, 0) { }
			
		ImageView( T* data, int width, int height, int stride )
			:	ImageViewBase<T>( data, width, height, stride) { }
			
		ImageView( T* data, int width, int height )
			:	ImageView( data, width, height, width ) { }
		
		void fill( T value ){
			for( auto row : *this )
				for( auto& val : row )
					val = value;
		}
		
		operator ConstImageView<T>() const
			{ return { this->data, this->w, this->h, this->stride() }; }
		
		ImageView<T> crop( int x, int y, int newWidth, int newHeight) const
			{ return { this->fromOffset( x, y ), newWidth, newHeight, this->stride() }; }
		
		void copyFrom( ConstImageView<T> other ){
			//if( this->stride() == other.stride() ){
				//TODO: Fast copy
			//}
			//else{
				//TODO: Line by line copy
				for( int iy=0; iy<this->height(); iy++ )
					std::memcpy((*this)[iy].begin(), other[iy].begin(), sizeof(T)*this->width());
			//}
		}
};


#endif

