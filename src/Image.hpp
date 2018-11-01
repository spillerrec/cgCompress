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

#ifndef IMAGE_HPP
#define IMAGE_HPP

#include "Format.hpp"
#include "SubImage.hpp"

#include "images/Rgba.hpp"

enum class DiffType{
		MATCHES //Pixel is the same as base
	,	DIFFERS //Pixel is different from base
	,	SHARED  //Pixel matches some bases, does not in others
};

using ImageMask = Image2<DiffType>;

class Image {
	private:
		SubImage img;
		ImageMask mask;
		
		QByteArray saved_data;
		
	public:
		/** \param [in] pos Offset of the image
		 *  \param [in] img The image data */
		Image( QPoint pos, ConstRgbaView img );
		
		/** \param [in] img QImage which will be converted to ARGB32 and positioned at {0,0} */
		Image( ConstRgbaView img=ConstRgbaView() ) : Image( {0,0}, img ) { }
		
		Image( const Image& other )
			:	img(other.img), mask(copy(other.mask))
			,	saved_data(other.saved_data)
			{ }
		
		Image& operator=( const Image& other ){
			img = other.img;
			mask = copy(other.mask);
			saved_data = other.saved_data;
			return *this;
		}
		
	private:
		Image( SubImage img, ImageMask mask ) : img(img), mask(std::move(mask)) { }
		Image newMask( ConstImageView<DiffType> mask ) const{ return Image( img, copy(mask) ); }
		
		QList<Image> segment() const;
		/*QList<Image> diff_segment( Image diff ) const;*/
		/** \return true if *this* overlaps *other* */
		/*bool overlaps( Image other ) const{
			return QRect( pos, size ).intersects( QRect( other.pos, other.size ) );
		}
		QSize size() const{ return mask.isNull() ? img.size() : mask.size(); }
		*/
	public:
		
		/** \return true if the image is valid */
		bool is_valid() const{ return !img.size().isEmpty(); }
		
		/** \return The offset of the image */
		QPoint get_pos() const{ return img.offset(); }
		
		/** \return The image data */
		ConstRgbaView view() const{ return img.view(); }
		
		/** Save the image to the file system
		 *  \param [in] path The location on the file system
		 *  \param [in] format The format used for compression
		 *  \return true if successful */
		bool save( QString path, Format format ) const{ return format.save( remove_transparent(), path ); }
		
		/** Save the image to a memory buffer
		 *  \param [in] format The compression format to use
		 *  \return The image in compressed form */
		QByteArray to_byte_array( Format format, bool keep_alpha=true ) const
			{ return saved_data.size() > 0 ? saved_data : format.to_byte_array( remove_transparent(), keep_alpha ); }
		
		/** Create a clipped version of the image
		 *  \param [in] x The amount to remove from the left
		 *  \param [in] y The amount to remove from the top
		 *  \param [in] width The width to include
		 *  \param [in] height The height to include
		 *  \return The clipped image */
		Image sub_image( int x, int y, int width, int height ) const{
			QSize newSize( std::min( x+width,  x+mask.width()  ) - x
			             , std::min( y+height, y+mask.height() ) - y );
			auto newMask = newSize.isNull() ? ImageMask() : copy( mask.crop( x,y, newSize.width(), newSize.height() ) );
			return Image( img.copy( {x,y}, newSize ), std::move(newMask) );
		}
		
		void combine( RgbaView base ) const;
		
		Image contain_both( Image diff ) const;
		struct SplitImage split_shared( const Image& other ) const;
		
		/** Calculates file size of the image; wrapper for Format::file_size()
		 *  \param [in] format Format used for compression
		 *  \param [in] p Precision of the file size calculation
		 *  \return The size of the image compressed */
		int compressed_size( Format format, Format::Precision p=Format::HIGH ) const;
		
		int save_compressed_size( Format format ){
			saved_data = to_byte_array( format );
			return saved_data.size();
		}
		int alpha_count() const;
		
		Image difference( Image img ) const;
		Image clean_alpha( int kernel_size, int threshold ) const;
		RgbaImage remove_transparent() const;
		Image auto_crop() const;
		
		Image optimize_filesize( Format format ) const;
		int estimate_compressed_size( Format format ) const;
		
		/** \param [in] other Image to compare against
		 *  \return true if images are interchangeable */
		bool operator==( const Image& other ) const
			{ return img == other.img && mask == other.mask; }
		
		bool mustKeepAlpha() const;
		static Image fromTransparent( ConstRgbaView img );
};

struct SplitImage{
	Image shared;
	Image first;
	Image second;
	int usefulness { 0 };
	int index{ -1 };
	SplitImage() : shared( {0,0}, {} ), first(shared), second(shared) {}
};

#endif

