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

#include "ListFunc.hpp"

#include <climits>
#include <iostream>
#include <string>
#include <map>
#include <utility>

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

int MultiImage::compressed_size(){
	using namespace std;
	std::map<pair<int,int>,int> sizes;
	
	for( auto frame : frames )
		for( auto converter : frame )
			sizes[make_pair(converter.get_from(), converter.get_to())] = converter.get_size();
	
	int total = 0;
	for( auto size : sizes )
		total += size.second;
	
	return total;
}

/** Find a converter to a frame not found, will be called recursively to find
 *  all frames.
 *  \param [in,out] used_converters The converters found will be added to this
 *  \param [in,out] frames The list of transformations to get the images. Each
 *                     transformation contains a the indexes of images to get
 *                     to the final image.
 *  \param [in] converters The available converters which may be used
 *  \param [in] amount The total amount of frames
 */
void MultiImage::add_converter( MultiImage& img, QList<Converter> converters, int amount ){
	if( amount == img.frames.size() )
		return;
	qDebug() << "add_conveter " << img.frames.size();
	
	QList<int> has;
	for( auto frame : img.frames ){
	qDebug() << "Has: " << frame.last().get_to();
		has << frame.last().get_to();
		}
	
	auto valid = filter( converters, [has](Converter c){ return !has.contains(c.get_from()) || has.contains(c.get_to()); } );
	qDebug() << "Valid:";
	for( auto converter : valid )
		qDebug() << converter.get_from() << " " << converter.get_to();
	auto best = minimum( valid, [](Converter c1, Converter c2){ return c1.get_size() < c2.get_size(); } );
	if( best >= valid.end() ){
		qDebug() << "oh no";
		return; //TODO:
	}
	
	//Remove converters to this image, we don't need them any more
	int to = (*best).get_to();
	filter_inpl( converters, [to](Converter c){ return c.get_to() == to; } );
	
	img.frames << (Frame() << *best);
	add_converter( img, converters, amount );
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


MultiImage MultiImage::optimized( Format format, QList<Image> originals, QList<Converter> converters, int base_image ){
	MultiImage multi( format, originals );
	multi.frames << ( QList<Converter>() << Converter( originals, base_image, base_image, format ) );
	//TODO: first frame contains originals[base_image]

	add_converter( multi, converters, originals.size() );
	sort( multi.frames, [](Frame first, Frame last){ return first.last().get_to() < last.last().get_to(); } );
	
	/*
	//Convert combined to multi.frames
	for( auto comb : combined ){
		Frame frame;
		frame << 0;
		for( int j=1; j<comb.size(); j++ ){
			int from = frame.last();
			from = (j == 1) ? base_image : comb[j-1];
			for( int i=0; i<used_converters.size(); i++ ){
				if( from == used_converters[i].get_from() && comb[j] == used_converters[i].get_to() ){
					frame << i+1;
					break;
				}
			}
		}
		multi.frames << frame;
	}
	
	// Try to reuse planes if possible
	for( int i=0; i<multi.primitives.size(); i++ ){
		//TODO: replace the order would avoid having to check?
		if( !multi.primitives[i].img.is_valid() )
			continue;
		for( int j=i+1; j<multi.primitives.size(); j++ ){
			if( !multi.primitives[j].img.is_valid() )
				continue;
			
			Image result = multi.primitives[i].img.contain_both( multi.primitives[j].img );
			if( result.is_valid() ){
				multi.primitives[j] = Primitive( Image( {0,0}, QImage() ), format );
				multi.replace_primitive( j, QList<int>() << i );
			}
		}
	}
	
	//TODO: combine stuff
	*/
	return multi;
}
MultiImage MultiImage::optimized( Format format, QList<Image> originals ){
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
	
	//Only do the first one, unless high precision have been selected
	int test_amount = 1;//(format.get_precision() == 0) ? originals.size() : 1;
	QList<int> testing;
	for( int best_start=0; best_start<test_amount; best_start++ )
		testing << best_start;
	
	//Try all originals as the base image, and pick the best one
	QList<MultiImage> imgs;
	{	ProgressBar progress( "Finding efficient solution", test_amount );
		imgs = map<MultiImage>( testing, [&](int i){
				progress.update();
				return optimized( format, originals, converters, i );
			} );
	}
	
	MultiImage img( *maximum( imgs, [](MultiImage& img1, MultiImage& img2){
			return img1.compressed_size() < img2.compressed_size();
		} ) );
	
	/*
	{	ProgressBar progress( "Optimizing images", img.primitives.size()-1 );
		for( int i=1; i<img.primitives.size(); i++, progress.update() )
			if( img.primitives[i].img.is_valid() )
				img.primitives[i].img = img.primitives[i].img.auto_crop().optimize_filesize( format );
	}
	*/
	
	//OraSaver( final_primitives, final_frames ).save( name + ".cgcompress", format );
	return img;
}

typedef Frame Frame2;
bool MultiImage::save( QString name ) const{
	QList<Image> primitives;
	QList<Converter> converters;
	
	qDebug() << "Frames: " << frames.size();
	for( auto frame : frames ){
		qDebug() << "Converters: " << frame.size();
		for( auto converter : frame ){
			primitives << converter.get_primitive().remove_transparent().auto_crop();
			converters << converter;
		}
	}
	
	for( auto current : converters )
			qDebug() << "Current: " << current.get_from() << " " << current.get_to();
	
	QList<Frame2> output;
	for( auto frame : frames ){
		QList<int> out;
		
		qDebug() << "Frame";
		auto current = frame.first();
		QList<int> pre; //All neeeded converters to this point
		while( true ){
			qDebug() << "Current: " << current.get_from() << " " << current.get_to();
			if( current.get_from() == current.get_to() ){
				pre << converters.indexOf( current );
				break;
			}
			else{
				pre << converters.indexOf( current );
				for( auto converter : converters )
					if( converter.get_to() == current.get_from() ){
						bool can_use = true;
						for( auto p : pre )
							if( converters[p] == converter )
								can_use = false;
						if( can_use ){
							current = converter;
		qDebug() << "Next: " << converters.indexOf( current );
							break;
						}
					}
			}
		}
		while( pre.size() > 0 )
			out << pre.takeLast();
		
		for( int i=1; i<frame.size(); i++ )
			out << converters.indexOf( frame[i] );
			
		Frame2 f( primitives );
		f.layers = out;
		output << f;
	}
	
	OraSaver( primitives, output ).save( name + ".cgcompress", format );
	
	return true;
	//TODO:
}

