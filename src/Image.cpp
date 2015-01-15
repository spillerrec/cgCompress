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

#include "Image.hpp"

#include <cmath>

#include <QPainter>
#include <QBuffer>

using namespace std;

/** Create a resized version of this image, will keep aspect ratio
 *  \param [in] size Maximum dimensions of the resized image
 *  \return The resized image */
Image Image::resize( int size ) const{
	size = min( size, img.width() );
	size = min( size, img.height() );
	QImage scaled = img.scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
	return Image( {0,0}, scaled );
}

static bool content_in_vertical_line( QImage img, int x ){
	for( int iy=0; iy<img.height(); iy++ )
		if( qAlpha( img.pixel(x,iy) ) != 0 )
			return true;
	return false;
}

/** Segment the image into several separate images, based on the alpha channel
 *  \return List of images which can be combined to into this image */
QList<Image> Image::segment() const{
	QList<Image> images;
	
	//Check vertical lines
	bool content = content_in_vertical_line( img, 0 );
	int first_line = 0;
	for( int ix=1; ix<img.width(); ix++ ){
		bool cur_content = content_in_vertical_line( img, ix );
		if( content == cur_content )
			continue;
		
		if( cur_content == true )
			first_line = ix;
		else
			images.append( sub_image( first_line,0, ix-first_line, img.height() ) );
		
		content = cur_content;
	}
	if( content )
		images.append( sub_image( first_line,0, img.width()-first_line, img.height() ) );
	//TODO: grab the last image if content touches the right edge
	
	//TODO: check horizontal lines
	//TODO: make recursive?
	
	//TODO: make sure this doesn't bloat the file size with many small images.
	//   A cost function which models the overhead could be used.
	
	return images;
}

/** \return This image where all transparent pixels are set to #000000 */
Image Image::remove_alpha() const{
	QImage output( img.convertToFormat(QImage::Format_ARGB32) );
	
	for( int iy=0; iy<output.height(); iy++ ){
		QRgb* out = (QRgb*)output.scanLine( iy );
		for( int ix=0; ix<output.width(); ix++ )
			if( qAlpha( out[ix] ) == 0 )
				out[ix] = qRgba( qRed(out[ix]),qGreen(out[ix]),qBlue(out[ix]),255 );
	}
			
	return Image( pos, output );
}

/** Make the area the other image covers transparent (colors are kept).
 *  \param [in] input The area to remove
 *  \return The resulting image */
Image Image::remove_area( Image input ) const{
	QImage output( img.convertToFormat(QImage::Format_ARGB32) );
	
	for( int iy=input.pos.y(); iy<input.pos.y()+input.img.height(); iy++ ){
		QRgb* out = (QRgb*)output.scanLine( iy );
		for( int ix=input.pos.x(); ix<input.pos.x()+input.img.width(); ix++ )
			out[ix] = qRgba( qRed(out[ix]),qGreen(out[ix]),qBlue(out[ix]),0 );
	}
			
	return Image( pos, output );
}

/** Segment based on how the images differs.
 *  \todo explain this better
 *  \param [in] diff The image used for differencing
 *  \return The segmented images */
QList<Image> Image::diff_segment( Image diff ) const{
	if( !overlaps( diff ) )
		return QList<Image>() << *this << diff;
	
	Image diff_diff = difference( diff );
	Image diff_diff2 = diff.difference( *this );
	
	Image new_diff1 = remove_area( diff_diff.auto_crop() ).remove_area( diff_diff2.auto_crop() );
	Image new_diff2 = diff.remove_area( diff_diff.auto_crop() ).remove_area( diff_diff2.auto_crop() );
	
	
	QList<Image> new_diffs;
	new_diffs << new_diff1;
	new_diffs << diff_diff2;
	new_diffs << remove_area( new_diff2.auto_crop() );
	new_diffs << diff.remove_area( new_diff1.auto_crop() );
	
	for( int i=new_diffs.size()-1; i>=0; i-- )
		if( new_diffs[i].auto_crop().img.size().isEmpty() )
			new_diffs.removeAt( i );
	
//	for( int i=0; i<new_diffs.size(); i++ )
//		new_diffs[i].save( QString( "diff %1.png" ).arg( i ) );
	return new_diffs;
}

/** Paint another image on top of this one
 *  \param [in] on_top Image to paint
 *  \return The combined image */
Image Image::combine( Image on_top ) const{
	QPoint tl{ min( pos.x(), on_top.pos.x() ), min( pos.y(), on_top.pos.y() ) };
	int width = max( pos.x()+img.width(), on_top.pos.x()+on_top.img.width() ) - tl.x();
	int height = max( pos.y()+img.height(), on_top.pos.y()+on_top.img.height() ) - tl.y();
	
	QImage output( width, height, QImage::Format_ARGB32 );
	output.fill( 0 );
	QPainter painter( &output );
	painter.drawImage( pos-tl, img );
	painter.drawImage( on_top.pos-tl, on_top.img );
	
	return Image( tl, output );
}

/** Paint another image on top of this one, modifying the image in-place.
 *  Must be contained in this image.
 *  \param [in] on_top Image to paint
 *  \return The combined image */
void Image::combineInplace( Image on_top ){
	QPainter painter( &img );
	painter.drawImage( on_top.pos-pos, on_top.img );
}

/** The difference between the two images
 *  \param [in] input The image to diff on, must have same dimensions
 *  \return The difference */
Image Image::difference( Image input ) const{
	//TODO: images must be the same size and at same point
	
	QImage output( img.convertToFormat(QImage::Format_ARGB32) );
	
	for( int iy=0; iy<output.height(); iy++ ){
		QRgb* out = (QRgb*)output.scanLine( iy );
		const QRgb* in = (const QRgb*)input.img.constScanLine( iy );
		for( int ix=0; ix<output.width(); ix++ ){
			if( qAlpha( in[ix] ) == 255 && qAlpha( out[ix] ) == 255 ){
				if( in[ix] == out[ix] )
					out[ix] = qRgba( qRed(in[ix]),qGreen(in[ix]),qBlue(in[ix]),0 );
				else
					out[ix] = in[ix];
			}
			else
				out[ix] = in[ix];
		}
	}
	
	return Image( {0,0}, output );
}

/** Checks if another image reduces the difference
 *  \todo Is this used anymore?
 *  \param [in] original The image wanted
 *  \param [in] diff The additional image to reduce difference
 *  \return true if *diff* reduces the difference */
bool Image::reduces_difference( Image original, Image diff ) const{
	//TODO: this and original must be larger than diff
	int balance = 0;
	int count = 0;
	
	int width = diff.pos.x() + diff.img.width();
	int height = diff.pos.y() + diff.img.height();
	for( int iy=diff.pos.y(); iy<height; iy++ ){
		const QRgb* base = (const QRgb*)         img.constScanLine( iy-pos.y() )          -          pos.x();
		const QRgb* org =  (const QRgb*)original.img.constScanLine( iy-original.pos.y() ) - original.pos.x();
		const QRgb* over = (const QRgb*)diff    .img.constScanLine( iy-diff.pos.y() )     -     diff.pos.x();
		
		for( int ix=diff.pos.x(); ix<width; ix++ ){
			if( qAlpha( over[ix] ) > 0 )
				count++;
			if( base[ix] != over[ix] ){
				if( qAlpha( over[ix] ) == 255 ){
					if( base[ix] == org[ix] )
						balance--;
					else if( over[ix] == org[ix] )
						balance++;
				}
			}
			//TODO: support 1-254 alpha
		}
	}
	
	return balance > count*0.0;
}

static bool color_equal( QRgb c1, QRgb c2 ){
	return qRed( c1 ) == qRed( c2 )
		&&	qGreen( c1 ) == qGreen( c2 )
		&&	qBlue( c1 ) == qBlue( c2 )
		;
}

/** Try to reset alpha to find an image which can simulate both images
 *  \param [in] input Another image
 *  \return An image which contains both images, or an invalid image on failure */
Image Image::contain_both( Image input ) const{
	//TODO: images must be the same size and at same point
	
	QImage output( img.convertToFormat(QImage::Format_ARGB32) );
	
	for( int iy=0; iy<output.height(); iy++ ){
		QRgb* out = (QRgb*)output.scanLine( iy );
		const QRgb* in = (const QRgb*)input.img.constScanLine( iy );
		for( int ix=0; ix<output.width(); ix++ ){
			if( qAlpha( in[ix] ) == 255 || qAlpha( out[ix] ) == 255 ){
				if( !color_equal( in[ix], out[ix] ) )
					return Image( {0,0}, QImage() ); //Can't contain both!
				else
					out[ix] = qRgba( qRed(out[ix]), qGreen(out[ix]), qBlue(out[ix]), 255 );
			}
			else{
				if( in[ix] != out[ix] )
					out[ix] = qRgba( 0,0,0,0 );
			}
		}
	}
	
	return Image( {0,0}, output );
}

/** Dilate the alpha channel to reduce salt&pepper noise
 *  \param [in] kernel_size How large area around each pixel should be considered
 *  \param [in] threshold How many pixels in the area must be set to enable this pixel
 *  \return The cleaned image */
Image Image::clean_alpha( int kernel_size, int threshold ) const{
	QImage output( img.convertToFormat(QImage::Format_ARGB32) );
	
	for( int iy=0; iy<output.height(); iy++ ){
		QRgb* out = (QRgb*)output.scanLine( iy );
		const QRgb* in = (const QRgb*)img.constScanLine( iy );
		for( int ix=0; ix<output.width(); ix++ )
			if( qAlpha( in[ix] ) == 0 ){
				out[ix] = qRgba( 0,0,0,0 );
				
				int amount = 0;
				int half = kernel_size / 2;
				int x_start = max( ix-half, 0 );
				int x_end = min( ix+kernel_size-half, img.width() );
				int y_start = max( iy-half, 0 );
				int y_end = min( iy+kernel_size-half, img.height() );
				for( int jy=y_start; jy<y_end; jy++ )
					for( int jx=x_start; jx<x_end; jx++ )
						if( qAlpha( img.pixel(jx,jy) ) == 255 )
							amount++;
				
				if( amount > threshold )
					out[ix] = qRgba( qRed(in[ix]), qGreen(in[ix]), qBlue(in[ix]), 255 );
				else
					out[ix] = in[ix];
			}
			else
				out[ix] = in[ix];
	}
			
	return Image( pos, output );
}

/** \return This image where all transparent pixels are set to transparent black **/
Image Image::remove_transparent() const{
	QImage output( img.convertToFormat(QImage::Format_ARGB32) );
	
	for( int iy=0; iy<output.height(); iy++ ){
		QRgb* out = (QRgb*)output.scanLine( iy );
		for( int ix=0; ix<output.width(); ix++ )
			if( qAlpha( out[ix] ) == 0 )
				out[ix] = qRgba( 0,0,0,0 );
	}
			
	return Image( pos, output );
}

/** \return This image, but with image data cropped to only contain non-transparent areas */
Image Image::auto_crop() const{
	int right=0, left=0;
	int top=0, bottom=0;
	
	//Decrease top
	for( ; top<img.height(); top++ ){
		const QRgb* row = (const QRgb*)img.constScanLine( top );
		for( int ix=0; ix<img.width(); ix++ )
			if( qAlpha( row[ix] ) != 0 )
				goto TOP_BREAK;
	}
TOP_BREAK:
	
	//Decrease bottom
	for( ; bottom<img.height(); bottom++ ){
		const QRgb* row = (const QRgb*)img.constScanLine( img.height()-1-bottom );
		for( int ix=0; ix<img.width(); ix++ )
			if( qAlpha( row[ix] ) != 0 )
				goto BOTTOM_BREAK;
	}
BOTTOM_BREAK:
	
	//Decrease left
	for( ; left<img.width(); left++ )
		for( int iy=0; iy<img.height(); iy++ )
			if( qAlpha( img.pixel(left,iy) ) != 0 )
				goto LEFT_BREAK;
LEFT_BREAK:
	
	//Decrease right
	for( ; right<img.width(); right++ )
		for( int iy=0; iy<img.height(); iy++ )
			if( qAlpha( img.pixel(img.width()-1-right,iy) ) != 0 )
				goto RIGHT_BREAK;
RIGHT_BREAK:
	
	return sub_image( left,top, img.width()-left-right, img.height()-top-bottom );
}

/** Minimize file size by cleaning the alpha, finds the best parameters
 *  \param [in] format Format used for compression
 *  \return Optimized image */
Image Image::optimize_filesize( Format format ) const{
	//skip images with no transparency
	unsigned black = 0, white = 0, total = img.width()*img.height();
	for( int iy=0; iy<img.height(); iy++ ){
		auto row = (const QRgb*)img.constScanLine( iy );
		for( int ix=0; ix<img.width(); ix++ ){
			auto val = qAlpha( row[ix] );
			if( val == 0 )
				black++;
			if( val == 255 )
				white++;
		}
	}
	
	//Start with the basic image
	Image best = remove_transparent();
	int best_size = best.compressed_size( format, Format::MEDIUM );
	
	if( black == total || white == total )
		return best;
	
	for( int i=0; i<7; i++ )
		for( int j=0; j<i*i; j++ ){
			Image current = clean_alpha( i, j ).remove_transparent();
			int size = current.compressed_size( format, Format::MEDIUM );
			if( size < best_size ){
				best_size = size;
				best = current;
			}
		}
	
	return best;
}

/** Minimize file size by cleaning the alpha, using 8x8 blocks to speed up empty areas
 *  \param [in] format Format used for compression
 *  \return Optimized image */
Image Image::optimize_filesize_blocks( Format format ) const{
	auto copy = *this;
	for( int iy=0; iy<img.height(); iy+=8 )
		for( int ix=0; ix<img.width(); ix+=8 ){
			auto block = copy.sub_image( ix, iy, 8, 8 ).optimize_filesize( format );
			copy.combineInplace( block );
		}
	
	return copy;
}


