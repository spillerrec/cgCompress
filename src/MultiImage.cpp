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

std::pair<QList<Frame>,int> MultiImage::lowest_cost( const QList<int>& costs, QList<QList<Frame>> all_frames, QList<int> used ){
	if( all_frames.isEmpty() ){
		//Return cost
		int cost = 0;
		for( auto layer : used )
			cost += costs[layer];
		return std::make_pair( QList<Frame>(), cost );
	}
	else{
		QList<Frame> frames = all_frames.first();
		all_frames.pop_front();
		
		QList<Frame> best_frame;
		int best_cost = INT_MAX;
		
		//Find best
		for( auto frame : frames ){
			QList<int> used_new = used;
			for( auto layer : frame.layers )
				if( !used_new.contains( layer ) )
					used_new.append( layer ); //TODO: could be slow, use sorted lists?
			
			auto result = lowest_cost( costs, all_frames, used_new );
			if( result.second < best_cost ){
				best_cost = result.second;
				best_frame = QList<Frame>();
				best_frame.append( frame );
				best_frame.append( result.first );
			}
		}
		
		return std::make_pair( best_frame, best_cost );
	}
}

QList<Image> MultiImage::diff_fast( int& amount ) const{
	QList<Image> sub_images;
	sub_images.append( originals.first() );
	amount = 1;
	
	//Find all possible differences
	for( int i=1; i<originals.size(); i++ )
		sub_images.append( originals[i-1].difference( originals[i] ) );
	
	return sub_images;
}

QList<Image> MultiImage::diff_linear( int& amount ) const{
	QList<Image> sub_images;
	sub_images.append( originals.first() );
	amount = 1;
	
	//Find all possible differences
	for( int i=0; i<originals.size(); i++ )
		for( int j=i+1; j<originals.size(); j++ )
			sub_images.append( originals[i].difference( originals[j] ) );
	
	return sub_images;
}

QList<Image> MultiImage::diff_all( int& amount ) const{
	QList<Image> sub_images( originals );
	amount = originals.size();
	
	//Find all possible differences
	for( int i=0; i<amount; i++ )
		for( int j=i+1; j<originals.size(); j++ ){
			sub_images.append( originals[i].difference( originals[j] ) );
			sub_images.append( originals[j].difference( originals[i] ) );
		}
	
	return sub_images;
}


QList<Frame> MultiImage::optimize() const{
	if( originals.size() == 0 )
		return QList<Frame>();
	
	//Differences
	int amount;
	QList<Image> diff_images = diff_linear( amount );
	
	//Segmentation
	QList<Image> sub_images;
	for( auto diff : diff_images )
		sub_images.append( diff.segment() );
	
	//Remove duplicates and auto-crop
	sub_images = remove_duplicates( sub_images );
	for( auto& sub : sub_images )
		sub = sub.auto_crop();
		
	for( int i=0; i<sub_images.size(); i++ )
		sub_images[i].remove_transparent().save( QString( "%1.webp" ).arg( i ) );
	
	//Generate all possible frames
	QList<QList<Frame>> all_frames;
	for( auto original : originals ){
		qDebug( "-------" );
		all_frames.append( Frame::generate_frames( sub_images, original, amount ) );
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
	QList<int> sizes;
	for( auto sub : sub_images )
		sizes.append( sub.compressed_size( "webp" ) );
	qDebug( "File sizes:" );
	for( auto size : sizes )
		qDebug( "\t%d bytes", size );
	
	//Optimize
	auto best = lowest_cost( sizes, all_frames );
	qDebug( "\nBest solution with size: %d bytes:", best.second );
	for( auto frame : best.first )
		frame.debug();
	return best.first;
}

