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

#include "MultiImage.hpp"

#include <climits>

//TODO: this is used in several places, find a fitting place to have this
template<typename T>
QList<T> remove_duplicates( QList<T> elements ){
	QList<T> list;
	
	for( auto element : elements )
		if( !list.contains( element ) )
			list.append( element );
	
	return list;
}

QList<Frame> MultiImage::lowest_cost( const QList<Image>& primitives, QList<QList<Frame>> all_frames ){
	//TODO:
	return QList<Frame>();
}

QList<Frame> MultiImage::optimize() const{
	if( originals.count() == 0 )
		return QList<Frame>();
	
	//First part is the original images
	QList<Image> sub_images( originals );
	
	//Find all possible differences
	for( int i=0; i<originals.count(); i++ )
		for( int j=i+1; j<originals.count(); j++ ){
			sub_images.append( originals[i].difference( originals[j] ).segment() );
			sub_images.append( originals[j].difference( originals[i] ).segment() );
		}
	
	//Remove duplicates and auto-crop
	sub_images = remove_duplicates( sub_images );
	for( auto& sub : sub_images )
		sub = sub.auto_crop();
		
	for( int i=0; i<sub_images.count(); i++ )
		sub_images[i].remove_transparent().save( QString( "%1.webp" ).arg( i ) );
	
	//Generate all possible frames
	QList<QList<Frame>> all_frames;
	for( auto original : originals ){
		qDebug( "-------" );
		all_frames.append( Frame::generate_frames( sub_images, original, originals.size() ) );
	}
	
	for( auto frames : all_frames ){
		qDebug( "-------" );
		for( auto frame : frames )
			frame.debug();
	}
	
	//Optimize sub_images for saving
	for( auto& sub : sub_images )
		sub = sub.remove_transparent();
	
	//Calculate file sizes
	QList<unsigned> sizes;
	for( auto sub : sub_images )
		sizes.append( sub.compressed_size( "webp" ) );
	qDebug( "File sizes:" );
	for( auto size : sizes )
		qDebug( "\t%d bytes", size );
	
	//Optimize
	return lowest_cost( sub_images, all_frames );
}

