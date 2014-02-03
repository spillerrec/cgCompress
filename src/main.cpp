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

#include <QCoreApplication>
#include <QStringList>
#include <QDebug>

#include <iostream>

#include "Image.hpp"
#include "Frame.hpp"

QList<Image> remove_dupes( QList<Image> images ){
	QList<Image> list;
	
	for( auto image : images ){
		if( !list.contains( image ) )
			list.append( image );
	}
	
	return list;
}

int main( int argc, char* argv[] ){
	QCoreApplication app( argc, argv );
	
	QStringList args = app.arguments();
	args.removeFirst();
	
	QList<Image> images;
	for( auto arg : args )
		images.append( Image( arg ) );
	
	QList<Image> sub_images;
	sub_images.append( images );
	for( int i=1; i<images.count(); i++ ){
		Image diff = images[i-1].difference( images[i] );
		sub_images.append( diff.segment() );
	}
	
	QList<Frame> frames = Frame::generate_frames( sub_images, images[images.size()-1], images.size() );
	qDebug( "Frames amout: %d", frames.count() );
	
	for( auto frame : frames ){
		static int i=0;
		std::cout << "Reconstruct " << i << ":\n";
		for( auto layer : frames[i].layers )
			std::cout << "\t" << layer;
		std::cout << "\n";
		i++;
		//frame.reconstruct().save( QString( "%1.png" ).arg( i++ ) );
	}
	
	/* Test combining
	for( int i=1; i<sub_images.count(); i++ )
		sub_images[i-1].auto_crop().combine( sub_images[i].auto_crop() ).save( QString( "com_%1.png" ).arg( i ) );
	
	sub_images = remove_dupes( sub_images );
	//*/
	
	for( int i=0; i<sub_images.count(); i++ )
		sub_images[i].auto_crop().remove_transparent().save( QString( "%1.png" ).arg( i ) );
	
	return 0;
}