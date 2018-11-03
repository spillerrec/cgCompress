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

#include <functional>


void combineMasks( ImageView<bool> mask, ConstImageView<bool> other ){
	mask.apply( other, [](bool m, bool o){ return m || o; } );
}

Image applyMask( ConstImageView<bool> mask, ConstRgbaView image ){
	mask.assertSizeMatch(image);
	auto func = []( bool m ){ return m ? DiffType::MATCHES : DiffType::DIFFERS; };
	return { {image}, transform( mask, func ) };
}


void setRefTo( ImageView<uint16_t> ref, ConstImageView<bool> mask, uint16_t value ){
	ref.apply(mask, [value]( uint16_t prev, bool mask){
		return mask ? value : prev;
	});
}

ImageSimMask getMaskFrom( ConstImageView<uint16_t> refs, uint16_t value ){
	return transform(refs, [value]( uint16_t ref){ return ref == value; } );
}


void ImageSimilarities::addImage( ConstRgbaView img ){
	//TODO: All this should be optimized by ignoring large empty areas
	
	//The indexes of other images sharing same pixels
	RefImage new_ref( img.width(), img.height() );
	//Initialize to point to itself, i.e. totally unique
	new_ref.fill( originals.size() );
	
	//The parts already covered by previous images
	ImageSimMask already_masked( img.width(), img.height() );
	already_masked.fill( false );
	
	for( unsigned i=0; i<originals.size(); i++ ){
		//NOTE: What about doing this in reverse, and simply ignore 'already_masked'?
		// Find the matching areas
		ImageSimMask current_mask = transform( originals[i], img, std::equal_to<>() );
		
		//Only keep the ares which are not already used
		current_mask.apply( already_masked, [](auto a, auto b){ return a && !b; } );
		
		//set current mask to 'i' in new_ref
		setRefTo( new_ref, current_mask, i );
		
		//Remove those areas from our mask
		already_masked.apply( current_mask, [](auto a, auto b){ return a || b; } );
	}
	
	originals.push_back( img );
	refs.emplace_back( std::move(new_ref) );
}

ImageSimMask ImageSimilarities::getMask( int id, int ref ){
	return getMaskFrom( refs.at(id), ref );
}

Image ImageSimilarities::getImagePart( int id, int ref )
	{ return applyMask( getMask( id, ref ), originals[id] ); }