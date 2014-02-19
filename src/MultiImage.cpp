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
#include "OraSaver.hpp"
#include "Converter.hpp"

#include <climits>
#include <iostream>
#include <string>

#include <QDebug>

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

QList<Image> segment_all( QList<Image> diffs ){
	diffs.removeFirst();
	for( int iter=0; iter<1; iter++ ){
		QList<Image> temp;
		for( int i=0; i<diffs.size(); i++ )
			for( int j=i+1; j<diffs.size(); j++ )
				temp << diffs[i].diff_segment( diffs[j] );
		diffs = remove_duplicates( temp );
	}
	
	for( auto& diff : diffs )
		diff = diff.remove_transparent().auto_crop();
	return remove_duplicates( diffs );
}


QList<Frame> MultiImage::optimize( QString name ) const{
	if( originals.size() == 0 )
		return QList<Frame>();
	
	//Differences
	int amount;
	QList<Image> diff_images = diff_linear( amount );
	
	//Segmentation
	QList<Image> sub_images;
	//*
	for( auto diff : diff_images )
		sub_images.append( diff);//.segment() );
	//diff_images[3].diff_segment( diff_images[4] );
	/*/
	for( int i=0; i<amount; i++ )
		sub_images.append( diff_images[i] );
	sub_images << segment_all( diff_images );
	//*/
		
	for( int i=0; i<sub_images.size(); i++ )
		sub_images[i].save( QString( "%1" ).arg( i ), Format( "png" ) );
	
	//Remove duplicates and auto-crop
	sub_images = remove_duplicates( sub_images );
	for( auto& sub : sub_images )
		sub = sub.auto_crop();
	sub_images = remove_duplicates( sub_images );
		
//	for( auto& sub : sub_images )
//		sub = sub.optimize_filesize( "webp" );
//	for( int i=amount; i<sub_images.size(); i++ )
//		sub_images[i] = sub_images[i].optimize_filesize( "webp" );
	
	/*
	for( int i=6; i<=7; i++ )
		for( int j=0; j<i*i; j++ )
			sub_images[3].clean_alpha( i, j ).remove_transparent().save( QString( "test/clean %1-%2.webp" ).arg( i ).arg( j ) );
	//*/
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
		sizes.append( sub.compressed_size( format ) );
	qDebug( "File sizes:" );
	for( auto size : sizes )
		qDebug( "\t%d bytes", size );
	
	//Optimize
	auto best = lowest_cost( sizes, all_frames );
	qDebug( "\nBest solution with size: %d bytes:", best.second );
	for( auto frame : best.first )
		frame.debug();
	OraSaver( sub_images, best.first ).save( name + ".cgcompress", format );
	return best.first;
}

void add_converter( QList<Converter>& used_converters, QList<QList<int>>& frames, const QList<Converter>& converters, int amount ){
	if( amount == frames.size() )
		return;
	
	QList<int> has;
	for( auto frame : frames )
		for( auto layer : frame )
			if( !has.contains( layer ) )
				has << layer;
	QList<int> used;
	for( auto frame : frames )
		used << frame.last();
	
	int best_converter = -1;
	int filesize = INT_MAX;
	for( int i=0; i<converters.size(); i++ ){
		if( has.contains( converters[i].get_from() ) && !used.contains( converters[i].get_to() ) )
			if( converters[i].get_size() < filesize ){
				best_converter = i;
				filesize = converters[i].get_size();
			}
	}
	if( best_converter == -1 )
		qFatal( "No converter could be found!" );
	
	QList<int> from_frame;
	for( auto frame : frames )
		if( frame.last() == converters[best_converter].get_from() ){
			from_frame = frame;
			break;
		}
	
	used_converters << converters[best_converter];
	//if( converters[best_converter].get_primitive().is_valid() )
	frames << ( from_frame << converters[best_converter].get_to() );
	
	add_converter( used_converters, frames, converters, amount );
}

class ProgressBar{
	private:
		int amount;
		int size;
		int count{ 0 };
		int written{ 0 };
		
	public:
		ProgressBar( std::string msg, int amount, int size ) : amount(amount), size(size){
			std::cout << msg << std::endl;
			for( int i=0; i<size; i++ )
				std::cout << "_";
			std::cout << std::endl;
		}
		~ProgressBar(){ std::cout << std::endl; }
		
		void update( int progress=1 ){
			for( count += progress; written < count*size/amount; written++ )
				std::cout << "X";
		}
};

QList<Frame> MultiImage::optimize2( QString name ) const{
	qDebug() << "Compressing " << name;
	int base = originals.size() - 1;
	
	QList<Converter> converters;
	{	ProgressBar progress( "Generating data", base*base + base, 60 );
		for( int i=0; i<originals.size(); i++ )
			for( int j=i+1; j<originals.size(); j++ ){
				converters << Converter( originals, i, j, format );
				progress.update();
				converters << Converter( originals, j, i, format );
				progress.update();
			}
	}
	
	QList<QByteArray> orgs_data;
	{	ProgressBar progress( "Finding best base image", originals.size(), 60 );
		for( auto org : originals ){
			orgs_data << org.to_byte_array( format );
			progress.update();
		}
	}
	
	int best_start = 0;
	int filesize = INT_MAX;
	for( int i=0; i<orgs_data.size(); i++ ){
		if( orgs_data[i].size() < filesize ){
			best_start = i;
			filesize = orgs_data[i].size();
		}
	}
	qDebug( "Smallest image is: %d", best_start );
	
	QList<QList<int>> combined;
	QList<Converter> used_converters;
	combined << ( QList<int>() << best_start  );
	
	add_converter( used_converters, combined, converters, originals.size() );
	qSort( combined.begin(), combined.end(), []( const QList<int>& first, const QList<int>& last ){ return first.last() < last.last(); } );
	
	QList<Image> primitives;
	primitives << originals[best_start];
	for( auto used : used_converters )
		primitives.append( used.get_primitive() );
	
	QList<Frame> frames;
	for( auto comb : combined ){
		Frame f( primitives );
		f.layers << 0;
		for( int j=1; j<comb.size(); j++ ){
			int from = f.layers.last();
			from = (j == 1) ? best_start : comb[j-1];
			for( int i=0; i<used_converters.size(); i++ ){
				if( from == used_converters[i].get_from() && comb[j] == used_converters[i].get_to() ){
					f.layers << i+1;
					break;
				}
			}
		}
		frames << f;
	}
	
	//* Try to reuse planes if possible
	for( int i=0; i<primitives.size(); i++ ){
		if( !primitives[i].is_valid() )
			continue;
		for( int j=i+1; j<primitives.size(); j++ ){
			if( !primitives[j].is_valid() )
				continue;
			
			Image result = primitives[i].contain_both( primitives[j] );
			if( result.is_valid() ){
				primitives[i] = result;
				primitives[j] = Image( {0,0}, QImage() );
				
				for( auto& frame : frames )
					for( auto& layer : frame.layers )
						if( layer == j )
							layer = i;
			}
		}
	}
	//*/
	
	for( int i=1; i<primitives.size(); i++ )
		if( primitives[i].is_valid() )
			primitives[i] = primitives[i].auto_crop()/*/.remove_transparent();/*/.optimize_filesize( format );//*/
	
	OraSaver( primitives, frames ).save( name + ".cgcompress", format );
	return frames;
}

