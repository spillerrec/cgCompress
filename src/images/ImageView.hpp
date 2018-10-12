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
class ConstImageView{
	private:
		const T* data;
		int width, height;
		int stride;
		
		const T* fromOffset( int x, int y ) const
			{ return data + y*stride + x; }
		
	public:
		ConstImageView( const T* data, int width, int height, int stride )
			:	data(data), width(width), height(height), stride(stride) { }
			
		ConstImageView( const T* data, int width, int height )
			:	ConstImageView( data, width, height, width ) { }
		
		auto operator[]( int y ) const
			{ return RowIt<T>( fromOffset(0,y), width ); }
			
		auto begin() const{ return RowIt<const T>( (*this)[  0   ], stride ); }
		auto end()   const{ return RowIt<const T>( (*this)[height], stride ); }
		
		ConstImageView crop( int x, int y, int newWidth, int newHeight) const
			{ return { fromOffset( x, y ), newWidth, newHeight, stride }; }
};

template<typename T>
class ImageView{
	private:
		T* data;
		int width, height;
		int stride;
		
		T* fromOffset( int x, int y ) const
			{ return data + y*stride + x; }
		
	public:
		ImageView( T* data, int width, int height, int stride )
			:	data(data), width(width), height(height), stride(stride) { }
			
		ImageView( T* data, int width, int height )
			:	ImageView( data, width, height, width ) { }
		
		auto operator[]( int y ) const
			{ return RowIt<T>( fromOffset(0,y), width ); }
			
		auto begin() const{ return RowIt<T>( (*this)[  0   ], stride ); }
		auto end()   const{ return RowIt<T>( (*this)[height], stride ); }
		
		ImageView crop( int x, int y, int newWidth, int newHeight) const
			{ return { fromOffset( x, y ), newWidth, newHeight, stride }; }
		
		void fill( T value ){
			for( auto row : *this )
				for( auto& val : row )
					val = value;
		}
		
		operator ConstImageView<T>() const
			{ return { data, width, height, stride }; }
};


#endif

