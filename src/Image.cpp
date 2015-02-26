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

//static int guid = 0; //For saving debug images

const auto TRANS_SET = qRgba( 255, 0, 255, 0 );
const auto TRANS_NONE = qRgba( 0, 0, 0, 0 );

const auto PIXEL_DIFFERENT = 0; //Pixel do not match with other image
const auto PIXEL_MATCH = 1;     //Pixel is the same as other image
const auto PIXEL_SHARED = 2;    //Pixel is the same, but must not be set as it differ in another image

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
/*
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
*/

/** \return This image where all transparent pixels are set to #000000 */
Image Image::discardTransparent() const{
	QImage output( img.convertToFormat(QImage::Format_ARGB32) );
	
	auto size = output.size();
	for( int iy=0; iy<size.height(); iy++ ){
		QRgb* out = (QRgb*)output.scanLine( iy );
		for( int ix=0; ix<size.width(); ix++ )
			if( qAlpha( out[ix] ) == 0 )
				out[ix] = qRgba( 0,0,0,0 );
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
/*
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
*/

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
	
	return Image( tl, output ); //TODO:
}

/** The difference between the two images
 *  \param [in] input The image to diff on, must have same dimensions
 *  \return The difference */
Image Image::difference( Image input ) const{
	//TODO: images must be the same size and at same point
	
	QImage mask( img.size(), QImage::Format_Indexed8 );
	mask.setColor( PIXEL_MATCH, qRgb(255, 0, 0) );
	mask.setColor( PIXEL_DIFFERENT, qRgb(0, 255, 0) );
	mask.setColor( PIXEL_SHARED, qRgb(0, 0, 255) );
	
	for( int iy=0; iy<img.height(); iy++ ){
		auto out      = (const QRgb*)      img.constScanLine( iy );
		auto in       = (const QRgb*)input.img.constScanLine( iy );
		auto out_mask = mask.scanLine( iy );
		for( int ix=0; ix<img.width(); ix++ )
			out_mask[ix] = (in[ix] == out[ix]) ? PIXEL_MATCH : PIXEL_DIFFERENT;
	}
	
	return input.newMask( mask );
}


/** Try to reset alpha to find an image which can simulate both images
 *  \param [in] input Another image
 *  \return An image which contains both images, or an invalid image on failure */
Image Image::contain_both( Image input ) const{
	//TODO: images must be the same size and at same point
	if( mask.isNull() ||input.mask.isNull() )
		return Image( {0,0}, QImage() );
	
	QImage mask_output( mask );
	
	for( int iy=0; iy<mask.height(); iy++ ){
		auto in1 = (const QRgb*)      img.constScanLine( iy +       pos.y() ) +       pos.x();
		auto in2 = (const QRgb*)input.img.constScanLine( iy + input.pos.y() ) + input.pos.x();
		
		auto mask1 =       mask.constScanLine( iy );
		auto mask2 = input.mask.constScanLine( iy );
		auto mask_out = mask_output.scanLine( iy );
		
		for( int ix=0; ix<mask.width(); ix++ ){
			auto pix1 = mask1[ix];
			auto pix2 = mask2[ix];
			auto& out = mask_out[ix];
			
			if( in1[ix] != in2[ix] ){
				//Pixel cannot be shared
				if( pix1 == PIXEL_DIFFERENT || pix2 == PIXEL_DIFFERENT )
					return Image( {0,0}, QImage() );
				
				out = PIXEL_SHARED;
			}
			else{
				if( pix1 == pix2 ) //No need to change
					out = pix1;
				else if( pix1 == PIXEL_MATCH ) //Other is the more specific
					out = pix2;
				else if( pix2 == PIXEL_MATCH ) //Other is the more specific
					out = pix1;
				else //One is shared, other is set, not allowed
					return Image( {0,0}, QImage() );
			}
		}
	}
	
//	       mask.save( "contain" + QString::number( guid ) + "in1.png" );
//	input .mask.save( "contain" + QString::number( guid ) + "in2.png" );
//	mask_output.save( "contain" + QString::number( guid ) + "out.png" );
//	guid++;
	
	return newMask( mask_output );
}

/** Dilate the alpha channel to reduce salt&pepper noise
 *  \param [in] kernel_size How large area around each pixel should be considered
 *  \param [in] threshold How many pixels in the area must be set to enable this pixel
 *  \return The cleaned image */
Image Image::clean_alpha( int kernel_size, int threshold ) const{
	QImage output( mask );
	int width = output.width(), height = output.height();
	
	std::vector<int> line( mask.width(), 0 );
	int half = kernel_size / 2;
	
	//Add/Remove line 'iy' to cache
	auto add    = [](int& val, unsigned char pix){ val = (pix == PIXEL_DIFFERENT) ? val+1 : val; };
	auto remove = [](int& val, unsigned char pix){ val = (pix == PIXEL_DIFFERENT) ? val-1 : val; };
	auto addLine = [&]( int iy ){
			auto in = mask.constScanLine( iy );
			for( int ix=0; ix<width; ix++ )
				add( line[ix], in[ix] );
		};
	auto removeLine = [&]( int iy ){
			auto in = mask.constScanLine( iy );
			for( int ix=0; ix<width; ix++ )
				remove( line[ix], in[ix] );
		};
	
	//Initialize cache
	for( int iy=0; iy<std::min(half,height); iy++ )
		addLine( iy );
	
	for( int iy=0; iy<height; iy++ ){
		auto out = output.scanLine( iy );
		auto in  = mask.constScanLine( iy );
		
		int kernel = 0;
		for( int ix=0; ix<std::min(half,width); ix++ )
			add( kernel, line[ix] );
		
		for( int ix=0; ix<width; ix++ ){
			if( in[ix] == PIXEL_MATCH )
				if( kernel > threshold )
					out[ix] = PIXEL_DIFFERENT;
			
			if( ix > 0 )
				remove( kernel, in[ix-1] );
			if( ix+1 < width )
				add( kernel, in[ix+1] );
		}
		
		if( iy > 0 )
			removeLine( iy-1 );
		if( iy+1 < height )
			addLine( iy+1 );
	}
			
	return newMask( output );
}

/** \return This image where all transparent pixels are set to transparent black **/
Image Image::remove_transparent() const{
	if( mask.isNull() )
		return *this;
	
	QImage output( img.copy( {pos, mask.size()} ).convertToFormat(QImage::Format_ARGB32) );
	int width = output.width(), height = output.height();
	
	for( int iy=0; iy<height; iy++ ){
		auto out = (QRgb*)output.scanLine( iy );
		auto out_mask = mask.constScanLine( iy );
			
		for( int ix=0; ix<width; ix++ )
			if( out_mask[ix] != PIXEL_DIFFERENT )
				out[ix] = TRANS_SET;
	}
			
	return Image( pos, output );
}

template<typename Func>
void find_auto_crop( QImage img, int &right, int &left, int &top, int &bottom, Func f ){
	//Decrease top
	for( ; top<img.height(); top++ ){
		auto row = img.constScanLine( top );
		for( int ix=0; ix<img.width(); ix++ )
			if( f( row[ix] ) )
				goto TOP_BREAK;
	}
TOP_BREAK:
	
	//Decrease bottom
	for( ; bottom<img.height(); bottom++ ){
		auto row = img.constScanLine( img.height()-1-bottom );
		for( int ix=0; ix<img.width(); ix++ )
			if( f( row[ix] ) )
				goto BOTTOM_BREAK;
	}
BOTTOM_BREAK:
	
	//Decrease left
	for( ; left<img.width(); left++ )
		for( int iy=0; iy<img.height(); iy++ )
			if( f( img.pixelIndex(left,iy) ) )
				goto LEFT_BREAK;
LEFT_BREAK:
	
	//Decrease right
	for( ; right<img.width(); right++ )
		for( int iy=0; iy<img.height(); iy++ )
			if( f( img.pixelIndex(img.width()-1-right,iy) ) )
				goto RIGHT_BREAK;
RIGHT_BREAK:
	return; //Whaat...
}

/** \return This image, but with image data cropped to only contain non-transparent areas */
Image Image::auto_crop() const{
	if( mask.isNull() )
		return *this;
	
	int right=0, left=0;
	int top=0, bottom=0;
	find_auto_crop( mask, right, left, top, bottom, [](unsigned i){ return i == PIXEL_DIFFERENT; } );
	
	return sub_image( left,top, img.width()-left-right, img.height()-top-bottom );
}

/** Minimize file size by cleaning the alpha, finds the best parameters
 *  \param [in] format Format used for compression
 *  \return Optimized image */
Image Image::optimize_filesize( Format format ) const{
	auto copy = auto_crop();
	
	//skip images with no transparency
	unsigned changeable = 0;
	for( int iy=0; iy<copy.mask.height(); iy++ ){
		auto row = copy.mask.constScanLine( iy );
		for( int ix=0; ix<copy.mask.width(); ix++ )
			if( row[ix] == PIXEL_MATCH )
				changeable++;
	}
	if( changeable > 0 )
		return copy.remove_transparent();
	
	//Start with the basic image
	Image best = copy;
	int best_size = best.compressed_size( format, Format::MEDIUM );
	
	for( int i=0; i<7; i++ )
		for( int j=0; j<i*i; j++ ){
			Image current = copy.clean_alpha( i, j );
			int size = current.compressed_size( format, Format::MEDIUM );
			if( size < best_size ){
				best_size = size;
				best = current;
			}
		}
	
	return best.remove_transparent();
}

