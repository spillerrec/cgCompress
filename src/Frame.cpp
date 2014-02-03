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


Image Frame::reconstruct() const{
	Image image( QPoint(0,0), QImage() );
	for( auto layer : layers )
		image = image.combine( primitives[layer] );
	return image;
}

QList<Frame> Frame::generate_frames( QList<Image>& primitives, Image original, int start ){
	QList<Frame> results;
	
	for( int i=0; i<start; i++ ){
		qDebug( "running" );
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
	
	return results;
}

