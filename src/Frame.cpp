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

#include <iostream>

//TODO: this is used in several places, find a fitting place to have this
template<typename T>
QList<T> remove_duplicates( QList<T> elements ){
	QList<T> list;
	
	for( auto element : elements )
		if( !list.contains( element ) )
			list.append( element );
	
	return list;
}

bool Frame::operator==( const Frame& other ) const{
	if( layers.count() != other.layers.count() )
		return false;
	
	//This also matches reshuffled versions, which are technically different.
	//However they do produce equal output, and the file size is the same, so we don't care.
	for( auto layer : other.layers )
		if( !layers.contains( layer ) )
			return false;
	
	return true;
}

Image Frame::reconstruct() const{
	Image image( QPoint(0,0), QImage() );
	for( auto layer : layers )
		image = image.combine( primitives[layer] );
	return image;
}

void Frame::debug() const{
	std::cout << "Frame layers: ";
	for( auto layer : layers )
		std::cout << "\t" << layer;
	std::cout << "\n";
}

QList<Frame> Frame::generate_frames( QList<Image>& primitives, Image original, int start ){
	QList<Frame> results;
	
	for( int i=0; i<start; i++ ){
		qDebug( "Generating frames: %d of %d", i+1, start );
		Frame current( primitives );
		current.layers.append( i );
		results.append( generate_frames( primitives, original, start, current ) );
	}
	
	return results;
}


QList<Frame> Frame::generate_frames( QList<Image>& primitives, Image original, int start, Frame current ){
	QList<Frame> results;
	
	if( original == current.reconstruct() )
		results.append( current );
	else{
		for( int i=start; i<primitives.size(); i++ ){
			if( !current.layers.contains( i ) ){
				Frame add( current );
				add.layers.append( i );
				results.append( generate_frames( primitives, original, start, add ) );
			}
		}
	}
	
	return remove_duplicates( results );
}

