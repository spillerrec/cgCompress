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

#include <QImage>
#include <QByteArray>

#include "Format.hpp"

class Image {
	private:
		QPoint pos;
		QImage img;
		
	public:
		/** \param [in] path Load the image at **path** on the file system */
		Image( QString path ) : img( QImage(path).convertToFormat(QImage::Format_ARGB32) ) { }
		
		/** \param [in] pos Offset of the image
		 *  \param [in] img The image data */
		Image( QPoint pos, QImage img ) : pos(pos), img(img) { }
		
		/** \return true if the image is valid */
		bool is_valid() const{ return !img.size().isEmpty(); }
		
		/** \return The offset of the image */
		QPoint get_pos() const{ return pos; }
		
		/** \return The image data */
		QImage qimg() const{ return img; }
		
		/** Remove alpha */
		void removeAlpha()
			{ img = img.convertToFormat(QImage::Format_RGB32).convertToFormat(QImage::Format_ARGB32); }
		
		/** Save the image to the file system
		 *  \param [in] path The location on the file system
		 *  \param [in] format The format used for compression
		 *  \return true if successful */
		bool save( QString path, Format format ) const{ return format.save( img, path ); }
		
		/** Save the image to a memory buffer
		 *  \param [in] format The compression format to use
		 *  \return The image in compressed form */
		QByteArray to_byte_array( Format format ) const{ return format.to_byte_array( img ); }
		
		/** \return true if *this* overlaps *other* */
		bool overlaps( Image other ) const{
			return QRect( pos, img.size() ).intersects( QRect( other.pos, other.img.size() ) );
		}
		
		Image resize( int size ) const;
		
		/** Create a clipped version of the image
		 *  \param [in] x The amount to remove from the left
		 *  \param [in] y The amount to remove from the top
		 *  \param [in] width The width to include
		 *  \param [in] height The height to include
		 *  \return The clipped image */
		Image sub_image( int x, int y, int width, int height ) const{
			return Image( pos+QPoint(x,y), img.copy( x,y, width,height ) );
		}
		
		QList<Image> segment() const;
		
		Image remove_alpha() const;
		QList<Image> diff_segment( Image diff ) const;
		
		Image combine( Image on_top ) const;
		void combineInplace( Image on_top );
		
		Image contain_both( Image diff ) const;
		
		/** Calculates file size of the image; wrapper for Format::file_size()
		 *  \param [in] format Format used for compression
		 *  \param [in] p Precision of the file size calculation
		 *  \return The size of the image compressed */
		int compressed_size( Format format, Format::Precision p=Format::HIGH ) const{
			return format.file_size( img, p );
		}
		Image difference( Image img ) const;
		Image remove_area( Image img ) const;
		bool reduces_difference( Image original, Image diff ) const;
		Image clean_alpha( int kernel_size, int threshold ) const;
		Image remove_transparent() const;
		Image auto_crop() const;
		
		Image optimize_filesize( Format format ) const;
		Image optimize_filesize_blocks( Format format ) const;
		
		/** \param [in] other Image to compare against
		 *  \return true if images are interchangeable */
		bool operator==( const Image& other ) const{
			return pos == other.pos && img == other.img;
		}
};

#endif

