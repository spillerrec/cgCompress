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
#include <QTime>

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

QList<Frame> Frame::optimize_list( QList<Frame> list ){
	QList<Frame> optimized;
	
	for( auto frame : list ){
		bool add = true;
		int replaced = 0;
		bool dupes = false;
		for( auto& optim : optimized ){
			if( optim == frame ){
				add = false;
				break;
			}
			
			auto frame_size = [](const Frame& a, const Frame& b){ return a.layers.size() < b.layers.size(); };
			Frame small = std::min( frame, optim, frame_size );
			Frame large = std::max( optim, frame, frame_size ); //TODO: make sure small != large
			
			bool equal = true;
			for( auto layer : small.layers )
				if( !large.layers.contains( layer ) )
					equal = false;
			
			if( equal ){
				if( replaced > 0 )
					dupes = true;
				optim = small;
				add = false;
				replaced++;
			}
			
		}
		
		if( add )
			optimized.append( frame );
		
		if( dupes )
			optimized = remove_duplicates( optimized );
	}
	
	return optimized;
}

Image Frame::reconstruct() const{
	if( layers.size() == 0 )
		return Image( QPoint(0,0), QImage() );
	
	Image image( primitives[layers[0]] );
	for( int i=1; i<layers.size(); i++ )
		image = image.combine( primitives[layers[i]] );
	return image;
}

void Frame::debug() const{
	std::cout << "Frame layers: ";
	for( auto layer : layers )
		std::cout << "\t" << layer;
	std::cout << "\n";
}

QList<Frame> Frame::generate_frames( const QList<Image>& primitives, const Image& original, int start ){
	QList<Frame> results;
	
	for( int i=0; i<start; i++ ){
		qDebug( "Generating frames: %d of %d", i+1, start );
		Frame current( primitives );
		current.layers.append( i );
	QTime t;
	t.start();
		results.append( optimize_list( generate_frames( primitives, original, start, current, primitives[i] ) ) );
	qDebug( "generate_frames time: %d", t.elapsed() );
	}
	
	return results;
}


QList<Frame> Frame::generate_frames( const QList<Image>& primitives, const Image& original, int start, const Frame& current, const Image& reconstructed, int depth  ){
	QList<Frame> results;
	
//	for( int i=0; i<depth; i++ )
//		std::cout << "  ";
//	std::cout << current.layers.last() << "\n";
	
	if( original == reconstructed ){
		results.append( current );
//		for( auto layer : current.layers )
//			std::cout << layer << " ";
//		std::cout << "\n";
	}
	else{
		QList<Image> areas = reconstructed.difference( original ).segment();
		for( auto& area : areas )
			area = area.auto_crop();
		
		//Find the primitives we want to test this round
		QList<int> testing;
		for( int i=start; i<primitives.size(); i++ ){
			if( !current.layers.contains( i ) ){
				//Skip if this primitive does not affect wrong areas
				bool relevant = false;
				for( auto area : areas )
					if( area.overlaps( primitives[i] ) ){
						relevant = true;
						break;
					}
				if( !relevant )
					continue;
				
				//* Only test primitives which overlaps the ones in testing
				//The rest will be tried in later stages.
				//This lowers the amount of useless frames which are just rearranged versions of each other
				if( testing.size() != 0 ){
					bool relevant = false;
					for( auto test : testing )
						if( primitives[test].overlaps( primitives[i] ) ){
							relevant = true;
							break;
						}
					if( !relevant )
						continue;
				}
				//*/
				testing.append( i );
			}
		}
		
		for( auto i : testing ){
			if( depth == 1 )
				std::cout << "x";
				
				//Skip if it doesn't affect it positively
				if( reconstructed.reduces_difference( original, primitives[i] ) ){
					Frame add( current );
					add.layers.append( i );
					results.append( generate_frames( primitives, original, start, add, reconstructed.combine( primitives[i] ), depth+1 ) );
				}
		}
		
		if( depth == 1 )
			std::cout << "\n";
	}
	
	return results;
}
