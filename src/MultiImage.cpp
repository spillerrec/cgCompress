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

//TODO: this is used in several places, find a fitting place to have this
template<typename T>
QList<T> remove_duplicates( QList<T> elements ){
	QList<T> list;
	
	for( auto element : elements )
		if( !list.contains( element ) )
			list.append( element );
	
	return list;
}

QList<Layer> MultiImage::optimize() const{
	if( originals.count() == 0 )
		return QList<Layer>();
	
	QList<Image> sub_images;
	sub_images.append( originals );
	for( int i=1; i<originals.count(); i++ ){
		Image diff = originals[i-1].difference( originals[i] );
		sub_images.append( diff.segment() );
	}
	
	QList<Frame> frames = Frame::generate_frames( sub_images, originals[originals.size()-1], originals.size() );
	qDebug( "Frames amount: %d", frames.count() );
	
	for( auto frame : frames )
		frame.debug();
	
	
	for( int i=0; i<sub_images.count(); i++ )
		sub_images[i].auto_crop().remove_transparent().save( QString( "%1.png" ).arg( i ) );
		
	return QList<Layer>();
}

