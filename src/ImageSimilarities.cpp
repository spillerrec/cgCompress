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

#include "ImageSimilarities.hpp"
#include "Image.hpp"

#include <cassert>

const int MASK_TRUE  = 1;
const int MASK_FALSE = 0;

ImageSimMask::ImageSimMask( int width, int height )
	: mask( width, height, QImage::Format_Indexed8 ) { }
	
void ImageSimMask::combineMasks( ImageSimMask combine_with ){
	assert( size() == combine_with.size() );
	
	for( int iy=0; iy<height(); iy++ ){
		auto row   = scanLine( iy );
		auto input = combine_with.constScanLine( iy );
		
		for( int ix=0; ix<width(); ix++ )
			row[ix] = (row[ix] || input[ix]) ? MASK_TRUE : MASK_FALSE;
	}
}

RefImage::RefImage( int width, int height )
	:	refs( std::make_unique<uint16_t[]>( width * height ) )
	,	width(width), height(height)
	{ }

uint16_t* RefImage::getRow( int iy ){
	assert( iy >= 0 && iy < height );
	return refs.get() + iy*width;
}

void RefImage::setMaskTo( ImageSimMask mask, uint16_t value ){
	assert( mask.width() == width && mask.height() == height );
	
	for( int iy=0; iy<height; iy++ ){
		auto row = getRow( iy );
		auto m_row = mask.constScanLine( iy );
		
		for( int ix=0; ix<width; ix++ )
			row[ix] = (m_row[ix] == MASK_TRUE) ? value : row[ix];
	}
}
void RefImage::fill( uint16_t value ){
	auto total_size = width*height;
	for( int i=0; i<total_size; i++ )
		refs[i] = value;
}

ImageSimMask RefImage::getMaskOf( uint16_t value ){
	ImageSimMask mask( width, height );
	
	for( int iy=0; iy<height; iy++ ){
		auto row = getRow( iy );
		auto out = mask.scanLine( iy );
		for( int ix=0; ix<width; ix++ )
			out[ix] = (row[ix] == value) ? MASK_TRUE : MASK_FALSE;
	}
	
	//TODO: Return null mask if all false
	return mask;
}

static ImageSimMask createMask( QImage img1, QImage img2, ImageSimMask& mask ){
	//The mask could be used to ignore already masked areas
	assert( img1.size() == img2.size() );
	assert( img1.size() == mask.size() );
	
	ImageSimMask new_mask = mask;
	new_mask.fill( MASK_FALSE );
	
	for( int iy=0; iy<img1.height(); iy++ ){
		auto row1 = (const QRgb*)img1.constScanLine( iy );
		auto row2 = (const QRgb*)img2.constScanLine( iy );
		auto row_old = mask.scanLine( iy );
		auto row_new = new_mask.scanLine( iy );
		
		for( int ix=0; ix<img1.width(); ix++ ){
			if( (row_old[ix] == MASK_FALSE) && (row1[ix] == row2[ix]) ){
				row_new[ix] = MASK_TRUE;
				row_old[ix] = MASK_TRUE;
			}
		}
	}
	
	return new_mask;
}

static QRgb makeTransparent( QRgb color )
	{ return 0; }//qRgba( qRed( color ), qGreen( color ), qBlue( color ), 0 ); }

//TODO: Use 'Image' class to support transparency?
Image ImageSimMask::apply( QImage image ) const{
	assert( image.size() == size() );
	
	for( int iy=0; iy<image.height(); iy++ ){
		auto row = (QRgb*)image.scanLine( iy );
		auto in  = constScanLine( iy );
		for( int ix=0; ix<image.width(); ix++ )
			if( in[ix] == MASK_FALSE )
				row[ix] = makeTransparent( row[ix] );
	}
	
	return Image::fromTransparent( image );
}


void ImageSimilarities::addImage( QImage img ){
	//TODO: All this should be optimized by ignoring large empty areas
	
	//The parts already covered by previous images
	ImageSimMask already_masked( img.size() );
	already_masked.fill( MASK_FALSE );
	
	//The indexes of other images sharing same pixels
	RefImage new_ref( img.width(), img.height() );
	//Initialize to point to itself, i.e. totally unique
	new_ref.fill( originals.size() );
	
	for( unsigned i=0; i<originals.size(); i++ ){
		auto current_mask = createMask( originals[i], img, already_masked );
		//set current mask to 'i' in new_ref
		new_ref.setMaskTo( current_mask, i );
	}
	
	originals.push_back( img );
	refs.emplace_back( std::move(new_ref) );
}

ImageSimMask ImageSimilarities::getMask( int id, int ref ){
	assert( id >= 0 && id < int(originals.size()) );
	
	return refs[id].getMaskOf(ref);
}

Image ImageSimilarities::getImagePart( int id, int ref )
	{ return getMask( id, ref ).apply( originals[id] ); }