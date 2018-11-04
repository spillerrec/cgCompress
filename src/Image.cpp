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
#include "FileSizeEval.hpp"
#include "formats/FormatWebP.hpp"
#include "decoder/OraHandler.hpp"

#include <cmath>
#include <algorithm>

using namespace std;

const auto TRANS_SET = Rgba( 255, 0, 255, 0 );


Image::Image( QPoint pos, ConstRgbaView img )
	:	img(img, pos)
	, mask(ImageMask( img.width(), img.height(), DiffType::DIFFERS )) //TODO: avoid mask?
	{ }

static bool content_in_vertical_line( ConstRgbaView img, int x ){
	for( int iy=0; iy<img.height(); iy++ )
		if( img[iy][x].a != 0 )
			return true;
	return false;
}

//TODO: Unused?
/** Segment the image into several separate images, based on the alpha channel
 *  \return List of images which can be combined to into this image */
std::vector<Image> Image::segment() const{
	std::vector<Image> images;
	
	//Check vertical lines
	bool content = content_in_vertical_line( img.view(), 0 );
	int first_line = 0;
	for( int ix=1; ix<img.width(); ix++ ){
		bool cur_content = content_in_vertical_line( img.view(), ix );
		if( content == cur_content )
			continue;
		
		if( cur_content == true )
			first_line = ix;
		else
			images.push_back( sub_image( first_line,0, ix-first_line, img.height() ) );
		
		content = cur_content;
	}
	if( content )
		images.push_back( sub_image( first_line,0, img.width()-first_line, img.height() ) );
	//TODO: grab the last image if content touches the right edge
	
	//TODO: check horizontal lines
	//TODO: make recursive?
	
	//TODO: make sure this doesn't bloat the file size with many small images.
	//   A cost function which models the overhead could be used.
	
	return images;
}


/** Segment based on how the images differs.
 *  \todo explain this better
 *  \param [in] diff The image used for differencing
 *  \return The segmented images */
/*
QList<Image> Image::diff_segment( Image diff ) const{
	if( !overlaps( diff ) )
		return QList<Image>() << *this << diff;
	
//remove_area removed, description below:
//	Make the area the other image covers transparent (colors are kept).
 // \param [in] input The area to remove
 // \return The resulting image
	
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

/** Paint this image onto 'base'
 *  \param [in] base The image to blend this image onto
 *  \return The combined image */
void Image::combine( RgbaView base ) const{
	OraDecoder::src_over( base, view(), get_pos().x(), get_pos().y() );
}

/** The difference between the two images
 *  \param [in] input The image to diff on, must have same dimensions
 *  \return The difference */
Image Image::difference( Image input ) const{
	if( get_pos() != input.get_pos() )
		throw std::runtime_error( "Image::difference - input pos/size does not match!" );
	
	return input.newMask( transform( img.view(), input.img.view()
		, [](auto a, auto b){ return a==b ? DiffType::MATCHES : DiffType::DIFFERS; } ) );
}


/** Try to reset alpha to find an image which can simulate both images
 *  \param [in] input Another image
 *  \return An image which contains both images, or an invalid image on failure */
Image Image::contain_both( Image input ) const{
	//TODO: images must be the same size and at same point
	if( !mask.valid() || !input.is_valid() )
		return {};
	//TODO: Check the behaviour of this
	
	ImageMask mask_output = copy( mask );
	
	for( int iy=0; iy<mask.height(); iy++ ){
		auto in1 =       img[iy];
		auto in2 = input.img[iy];
		
		auto mask1    =       mask [iy];
		auto mask2    = input.mask [iy];
		auto mask_out = mask_output[iy];
		
		for( int ix=0; ix<mask.width(); ix++ ){
			auto pix1 = mask1[ix];
			auto pix2 = mask2[ix];
			auto& out = mask_out[ix];
			
			if( in1[ix] != in2[ix] ){
				//Pixel cannot be shared
				if( pix1 == DiffType::DIFFERS || pix2 == DiffType::DIFFERS )
					return {};
				
				out = DiffType::SHARED;
			}
			else{
				if( pix1 == pix2 ) //No need to change
					out = pix1;
				else if( pix1 == DiffType::MATCHES ) //Other is the more specific
					out = pix2;
				else if( pix2 == DiffType::MATCHES ) //Other is the more specific
					out = pix1;
				else //One is shared, other is set, not allowed
					return {};
			}
		}
	}
	
//	       mask.save( "contain" + QString::number( guid ) + "in1.png" );
//	input .mask.save( "contain" + QString::number( guid ) + "in2.png" );
//	mask_output.save( "contain" + QString::number( guid ) + "out.png" );
//	guid++;
	
	return newMask( mask_output );
}


SplitImage Image::split_shared( const Image& input ) const{
	//TODO: images must be the same size and at same point
	if( !mask.valid() || !input.mask.valid() )
		return {};
	
	
	//Find the shared area of the two images
	ImageMask mask_shared = copy( mask );
	for( int iy=0; iy<mask.height(); iy++ ){
		auto in1 =       img[iy];
		auto in2 = input.img[iy];
		
		auto mask1 =       mask[iy];
		auto mask2 = input.mask[iy];
		auto mask_out = mask_shared[iy];
		
		for( int ix=0; ix<mask.width(); ix++ ){
			//If both images contains this pixel
			auto mask_match = (mask1[ix] == mask2[ix]) && (mask2[ix] == DiffType::DIFFERS);
			//And pixels are the same
			auto pixels_match = in1[ix] == in2[ix];
			
			//Then transfer it to the shared mask
			mask_out[ix] = ( mask_match && pixels_match ) ? DiffType::DIFFERS : DiffType::SHARED;
		}
	}
	
	//Function for removing the shared areas of the masks
	auto cut_mask = []( ConstImageView<DiffType> mask, ConstImageView<DiffType> cut ){
			auto out = copy( mask );
			for( int iy=0; iy<mask.height(); iy++ ){
				auto r_out = out[iy];
				auto r_cut = cut[iy];
				
				for( int ix=0; ix<mask.width(); ix++ )
					r_out[ix] = (r_cut[ix] == DiffType::DIFFERS) ? DiffType::SHARED : r_out[ix];
			}
			return out;
		};
	
	SplitImage result;
	result.shared = Image( this->img, copy(mask_shared) );
	result.first  = Image( this->img, cut_mask( this->mask, mask_shared ) );
	result.second = Image( input.img, cut_mask( input.mask, mask_shared ) );
	result.usefulness = -1;
	
	//if( result.shared.auto_crop().is_valid())
	//	if(!result.first.auto_crop().is_valid() || !result.second.auto_crop().is_valid())
	//		qFatal("Invalid split for images");
	
	return result;
}

/** Dilate the alpha channel to reduce salt&pepper noise
 *  \param [in] kernel_size How large area around each pixel should be considered
 *  \param [in] threshold How many pixels in the area must be set to enable this pixel
 *  \return The cleaned image */
Image Image::clean_alpha( int kernel_size, int threshold ) const{
	ImageMask output = copy( mask );
	int width = output.width(), height = output.height();
	
	std::vector<int> line( mask.width(), 0 );
	int half = kernel_size / 2;
	
	//Add/Remove line 'iy' to cache
	auto add    = [](int& val, DiffType pix){ val = (pix == DiffType::DIFFERS) ? val+1 : val; };
	auto remove = [](int& val, DiffType pix){ val = (pix == DiffType::DIFFERS) ? val-1 : val; };
	auto addLine = [&]( int iy ){
			auto in = mask[iy];
			for( int ix=0; ix<width; ix++ )
				add( line[ix], in[ix] );
		};
	auto removeLine = [&]( int iy ){
			auto in = mask[iy];
			for( int ix=0; ix<width; ix++ )
				remove( line[ix], in[ix] );
		};
	
	//Initialize cache
	for( int iy=0; iy<std::min(half,height); iy++ )
		addLine( iy );
	
	for( int iy=0; iy<height; iy++ ){
		auto out = output[iy];
		auto in  = mask[iy];
		
		int kernel = 0;
		for( int ix=0; ix<std::min(half,width); ix++ )
			kernel += (line[ix] > 0 ? 1 : 0);
		
		for( int ix=0; ix<width; ix++ ){
			if( in[ix] == DiffType::MATCHES )
				if( kernel > threshold )
					out[ix] = DiffType::DIFFERS;
			
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
RgbaImage Image::remove_transparent() const{
	if( !mask.valid() )
		return copy( view() ); //Nothing is transparent
	
	return transform( view(), mask.asConst()
		, [](Rgba img, DiffType m){ return (m == DiffType::DIFFERS) ? img : TRANS_SET; } );
}

struct ContentMap{
	std::vector<uint8_t> hor;
	std::vector<uint8_t> ver;
	ContentMap( ConstImageView<DiffType> mask );
};

	ContentMap::ContentMap( ConstImageView<DiffType> mask )
		:	hor( mask.width(), false )
		,	ver( mask.height(), false ){
		
		for( int iy=0; iy<mask.height(); iy++ ){
			auto row = mask[iy];
			for( int ix=0; ix<mask.width(); ix++ )
				if( row[ix] == DiffType::DIFFERS )
					hor[ix] = ver[iy] = true;
		}
	}
/** \return This image, but with image data cropped to only contain non-transparent areas */
Image Image::auto_crop() const{
	if( !mask.valid() )
		return Image( view() );
	
	//Build up lookup for horizontal and vertical lines
	ContentMap map( mask );
	
	//Find cropping size
	int w = map.hor.size(), h = map.ver.size();
	int x=0, y=0, width=w-1, height=h-1;
	for( ;    x < w  && !map.hor[  x   ]; x++      );
	for( ;    y < h  && !map.ver[  y   ]; y++      );
	for( ; width >=x && !map.hor[width ]; width--  );
	for( ; height>=y && !map.ver[height]; height-- );
	
	//Crop image
	return sub_image( x, y, width-x+1, height-y+1 );
}

/** Minimize file size by cleaning the alpha, finds the best parameters
 *  \param [in] format Format used for compression
 *  \return Optimized image */
Image Image::optimize_filesize( Format format ) const{
	auto copy = auto_crop();
	
	//skip images with no transparency
	unsigned changeable = 0;
	for( auto row : copy.mask )
		for( auto value : row )
			if( value == DiffType::MATCHES )
				changeable++;
	if( changeable == 0 )
		return copy;
	
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
	
	//Make sure filtering actually improved the situation
	if( copy.save_compressed_size( format ) > best.save_compressed_size( format ) )
		return best;
	else
		return copy;
}

int Image::compressed_size( Format format, Format::Precision p ) const{
	//Use low-precision if not high
	//NOTE: We cannot use the saved, as it is different
	if( format.get_precision() > 0 && p != Format::HIGH )
		return estimate_compressed_size( format );
	
	//If we already have compressed it, use that
	//TODO: Wrong if format have changed!
	if( saved_data.size() > 0 )
		return saved_data.size();
	
	//Calculate the gradient
	return format.file_size( remove_transparent(), p );
}

int Image::estimate_compressed_size( Format format ) const{
	//if( !mask.valid() )
	//	return format.file_size( remove_transparent(), Format::LOW );
	
	return FormatWebP::estimate_filesize( remove_transparent(), true );
	//return FileSize::image_gradient_sum( img, mask, PIXEL_DIFFERENT );
	return FileSize::lz4compress_size( remove_transparent() );
}

int Image::alpha_count() const{
	if( !mask.valid() )
		qFatal( "Image::alpha_count() not implemented for RGB" );
	
	int count = 0;
	for( auto row : mask )
		count += std::count( row.begin(), row.end(), DiffType::DIFFERS );
	
	return count;
}

bool Image::mustKeepAlpha() const{
	for( auto row : view() )
		for( auto pixel : row )
			if( (pixel.a != 255) && pixel != TRANS_SET )
				return true;
	
	return false;
}


Image Image::fromTransparent( ConstRgbaView img ){
	return Image( img ).newMask( transform( img, [](Rgba val)
		{ return (val.a != 0) ? DiffType::DIFFERS : DiffType::MATCHES; } ) );
}
