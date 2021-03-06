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

#include <QDebug>
#include <QImage>
#include <QByteArray>

#include "Format.hpp"
#include "SubQImage.hpp"

class Image {
	private:
		SubQImage img;
		QImage mask;
		
		QByteArray saved_data;
		
	public:
		/** \param [in] pos Offset of the image
		 *  \param [in] img The image data */
		Image( QPoint pos, QImage img );
		
		/** \param [in] img QImage which will be converted to ARGB32 and positioned at {0,0} */
		Image( QImage img ) : Image( {0,0}, img.convertToFormat(QImage::Format_ARGB32) ) { }
		
		/** \param [in] path Load the image at **path** on the file system */
		Image( QString path ) : Image( QImage(path) ) { }
		
	private:
		Image( SubQImage img, QImage mask ) : img(img), mask(mask) { }
		Image newMask( QImage mask ) const{ return Image( img, mask ); }
		/*
		QList<Image> segment() const;
		QList<Image> diff_segment( Image diff ) const;*/
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
		QImage qimg() const{ return img.get(); }
		
		/** Save the image to the file system
		 *  \param [in] path The location on the file system
		 *  \param [in] format The format used for compression
		 *  \return true if successful */
		bool save( QString path, Format format ) const{ return format.save( remove_transparent(), path ); }
		
		/** Save the image to a memory buffer
		 *  \param [in] format The compression format to use
		 *  \return The image in compressed form */
		QByteArray to_byte_array( Format format ) const
			{ return saved_data.size() > 0 ? saved_data : format.to_byte_array( remove_transparent() ); }
		
		Image resize( int size ) const;
		
		/** Create a clipped version of the image
		 *  \param [in] x The amount to remove from the left
		 *  \param [in] y The amount to remove from the top
		 *  \param [in] width The width to include
		 *  \param [in] height The height to include
		 *  \return The clipped image */
		Image sub_image( int x, int y, int width, int height ) const{
			QSize newSize( std::min( x+width,  x+mask.width()  ) - x
			             , std::min( y+height, y+mask.height() ) - y );
			auto newMask = newSize.isNull() ? QImage() : mask.copy( x,y, newSize.width(), newSize.height() );
			return Image( img.copy( {x,y}, newSize ), newMask );
		}
		
		Image combine( Image on_top ) const;
		
		Image contain_both( Image diff ) const;
		struct SplitImage split_shared( Image other ) const;
		
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
		Image remove_area( Image img ) const;
		Image clean_alpha( int kernel_size, int threshold ) const;
		QImage remove_transparent() const;
		Image auto_crop() const;
		
		Image optimize_filesize( Format format ) const;
		int estimate_compressed_size( Format format ) const;
		
		/** \param [in] other Image to compare against
		 *  \return true if images are interchangeable */
		bool operator==( const Image& other ) const
			{ return img == other.img && mask == other.mask; }
		
		bool mustKeepAlpha() const;
		static Image fromTransparent( QImage img );
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

