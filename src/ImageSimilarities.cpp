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


void combineMasks( ImageView<bool> mask, ConstImageView<bool> other ){
	mask.apply( other, [](bool m, bool o){ return m || o; } );
}

Image applyMask( ConstImageView<bool> mask, ConstRgbaView image ){
	mask.assertSizeMatch(image);
	ImageMask img_mask( mask.width(), mask.height() );
	img_mask.apply( mask, []( DiffType, bool m ){ return m ? DiffType::MATCHES : DiffType::DIFFERS; } );
	return { {image}, std::move(img_mask) };
}


void setRefTo( ImageView<uint16_t> ref, ConstImageView<bool> mask, uint16_t value ){
	ref.apply(mask, [value]( uint16_t prev, bool mask){
		return mask ? value : prev;
	});
}

ImageSimMask getMaskFrom( ConstImageView<uint16_t> refs, uint16_t value ){
	ImageSimMask mask( refs.width(), refs.height() );
	mask.apply(refs, [value]( bool, uint16_t ref){
		return ref == value;
	});
	return mask;
}

static ImageSimMask createMask( ConstRgbaView img1, ConstRgbaView img2, ImageSimMask& mask ){
	//The mask could be used to ignore already masked areas
	img1.assertSizeMatch(img2);
	img1.assertSizeMatch(mask);
	//assert( img1.size() == img2.size() );
	//assert( img1.size() == mask.size() );
	
	ImageSimMask new_mask( mask.width(), mask.height() );
	new_mask.fill( false );
	
	for( int iy=0; iy<img1.height(); iy++ ){
		auto row1 = img1[ iy ];
		auto row2 = img2[ iy ];
		auto row_old = mask[iy];
		auto row_new = new_mask[iy];
		
		for( int ix=0; ix<img1.width(); ix++ ){
			if( (row_old[ix] == false) && (row1[ix] == row2[ix]) ){
				row_new[ix] = true;
				row_old[ix] = true;
			}
		}
	}
	
	return new_mask;
}


void ImageSimilarities::addImage( ConstRgbaView img ){
	//TODO: All this should be optimized by ignoring large empty areas
	
	//The parts already covered by previous images
	ImageSimMask already_masked( img.width(), img.height() );
	already_masked.fill( false );
	
	//The indexes of other images sharing same pixels
	RefImage new_ref( img.width(), img.height() );
	//Initialize to point to itself, i.e. totally unique
	new_ref.fill( originals.size() );
	
	for( unsigned i=0; i<originals.size(); i++ ){
		auto current_mask = createMask( originals[i], img, already_masked );
		//set current mask to 'i' in new_ref
		setRefTo( new_ref, current_mask, i );
	}
	
	originals.push_back( img );
	refs.emplace_back( std::move(new_ref) );
}

ImageSimMask ImageSimilarities::getMask( int id, int ref ){
	return getMaskFrom( refs.at(id), ref );
}

Image ImageSimilarities::getImagePart( int id, int ref )
	{ return applyMask( getMask( id, ref ), originals[id] ); }