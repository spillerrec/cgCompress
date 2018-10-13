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
			
		RowIt<T>& operator*() const { return *this; }
		
};

template<typename T>
class ImageViewBase{
	protected:
		T* data;
		int w, h;
		int stride;
		
		T* fromOffset( int x, int y ) const
			{ return data + y*stride + x; }
		
	public:
		ImageViewBase( T* data, int width, int height, int stride )
			:	data(data), w(width), h(height), stride(stride) { }
			
		ImageViewBase( T* data, int width, int height )
			:	ImageViewBase<T>( data, width, height, width ) { }
		
		auto width()  const{ return w; }
		auto height() const{ return h; }
		
		auto operator[]( int y ) const
			{ return RowIt<T>( fromOffset(0,y), width(), stride ); }
			
		auto begin() const{ return (*this)[0]; }
		auto end()   const{ return (*this)[h]; }
};

template<typename T>
class ConstImageView : public ImageViewBase<const T>{
	public:
		ConstImageView( const T* data, int width, int height, int stride )
			:	ImageViewBase<const T>( data, width, height, stride) { }
			
		ConstImageView( const T* data, int width, int height )
			:	ConstImageView( data, width, height, width ) { }
			
		
		ConstImageView<T> crop( int x, int y, int newWidth, int newHeight) const
			{ return { this->fromOffset( x, y ), newWidth, newHeight, this->stride }; }
};

template<typename T>
class ImageView : public ImageViewBase<T>{
	public:
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
			{ return { this->data, this->w, this->h, this->stride }; }
		
		ImageView<T> crop( int x, int y, int newWidth, int newHeight) const
			{ return { this->fromOffset( x, y ), newWidth, newHeight, this->stride }; }
};


#endif

