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

/** Creates a progress bar on stdout with a title. Scope is used to stop the
 *  progress bar, do not output anything to stdout until the destructor is
 *  called */
class ProgressBar{
	private:
		int amount;
		int size;
		int count{ 0 };
		int written{ 0 };
		
	public:
		/** Create the progress bar
		 *  
		 *  \param [in] msg A title to be displayed together with the progress
		 *  \param [in] amount The amount of task to be done
		 *  \param [in] size The width of the progress bar
		 */
		ProgressBar( std::string msg, int amount, int size=60 ) : amount(amount), size(size){
			//Print slightly fancy header with centered text
			msg += " (" + std::to_string( amount ) + ")";
			int left = size - msg.size();
			
			for( int i=0; i<size; i++ )
				std::cout << "_";
			
			std::cout << std::endl << "|";
			for( int i=1; i<left/2; i++ )
				std::cout << " ";
			std::cout << msg;
			for( int i=1; i<left-left/2; i++ )
				std::cout << " ";
			std::cout << "|" << std::endl;
		}
		/** Stops and closes the progress bar */
		~ProgressBar(){ std::cout << std::endl; }
		
		/** Advance the progress
		 * \param [in] progress How much progress that have been made
		 */
		void update( int progress=1 ){
			for( count += progress; written < count*size/amount; written++ )
				std::cout << "X";
		}
};

QList<Frame> MultiImage::optimize( QString name ) const{
	qDebug() << "Compressing " << name;
	int base = originals.size() - 1;
	
	QList<Converter> converters;
	{	ProgressBar progress( "Generating data", base*base + base );
		for( int i=0; i<originals.size(); i++ )
			for( int j=i+1; j<originals.size(); j++ ){
				converters << Converter( originals, i, j, format );
				progress.update();
				converters << Converter( originals, j, i, format );
				progress.update();
			}
	}
	
	//Try all originals as the base image, and pick the best one
	int best_size = INT_MAX;
	QList<Image> final_primitives;
	QList<Frame> final_frames;
	
	//Only do the first one, unless high precision have been selected
	int test_amount = (format.get_precision() == 0) ? originals.size() : 1;
	{	ProgressBar progress( "Finding efficient solution", test_amount, 60 );
		for( int best_start=0; best_start<test_amount; best_start++, progress.update() ){
			QList<QList<int>> combined;
			QList<Converter> used_converters;
			combined << ( QList<int>() << best_start  );
			
			add_converter( used_converters, combined, converters, originals.size() );
			qSort( combined.begin(), combined.end(), []( const QList<int>& first, const QList<int>& last ){ return first.last() < last.last(); } );
			
			QList<Image> primitives;
			QList<int> file_sizes; //File size cache
			primitives << originals[best_start];
			file_sizes << originals[best_start].compressed_size( format, Format::MEDIUM );
			for( auto used : used_converters ){
				primitives.append( used.get_primitive() );
				file_sizes << used.get_size();
			}
			
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
			
			// Try to reuse planes if possible
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
						file_sizes[j] = 0;
						
						for( auto& frame : frames )
							for( auto& layer : frame.layers )
								if( layer == j )
									layer = i;
					}
				}
			}
			
			//Evaluate file size and overwrite old solution if better
			int filesize = std::accumulate( file_sizes.begin(), file_sizes.end(), 0 );
			if( filesize < best_size ){
				best_size = filesize;
				final_primitives = primitives;
				final_frames = frames;
			}
		}
	}
	
	{	ProgressBar progress( "Optimizing images", final_primitives.size()-1, 60 );
		for( int i=1; i<final_primitives.size(); i++, progress.update() )
			if( final_primitives[i].is_valid() )
				final_primitives[i] = final_primitives[i].auto_crop().optimize_filesize( format );
	}
	
	OraSaver( final_primitives, final_frames ).save( name + ".cgcompress", format );
	return final_frames;
}

