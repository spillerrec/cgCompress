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

#include "Frame.hpp"

/** \return The image this frame represent */
RgbaImage Frame::reconstruct() const{
	if( layers.size() == 0 )
		return {};
	
	RgbaImage image = copy( primitives[layers[0]].view() );
	//TODO: check that base image is the largest?
	for( size_t i=1; i<layers.size(); i++ )
		primitives[layers[i]].combine( image );
	return image;
}

void Frame::update_ids( int from, std::vector<int> to ){
	std::vector<int> new_layers;
	
	for( auto layer : layers ){
		if( layer != from )
			new_layers.push_back( layer );
		else
			for( auto l : to )
				new_layers.push_back( l );
	}
	
	layers = new_layers;
}

void Frame::remove_pointless_layers(){ //TODO:
	/*//Final result, anything we do must not change this!
	auto truth = reconstruct();
	
	//Try to remove each layer one at a time
	for( int k=1; k<layers.size(); k++ ){ //TODO: make it possible to remove the first layer as well
		Image image( primitives[layers[0]] );
		for( int i=1; i<layers.size(); i++ )
			if( i != k && layers[i] != -1 )
				image = image.combine( primitives[layers[i]] );
		
		//Did it cause any change? If not then we can remove it
		if( image == truth ){
			qDebug( "Found pointless layer %d", layers[k] );
			layers[k] = -1;
		}
	}
	
	//Properly remove the layers we marked as pointless
	layers.removeAll( -1 );*/
}


