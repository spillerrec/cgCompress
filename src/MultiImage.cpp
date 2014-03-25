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


QList<Converter> MultiImage::generateConverters( Format format, QList<Image> originals ){
	int base = originals.size() - 1;
	
	QList<Converter> converters;
	{	ProgressBar progress( "Generating data", base*base + base );
		for( int i=0; i<originals.size(); i++ ){
			converters << Converter( originals, {{i, i}}, format );
			for( int j=i+1; j<originals.size(); j++ ){
				converters << Converter( originals, {{i, j}}, format );
				progress.update();
				converters << Converter( originals, {{j, i}}, format );
				progress.update();
			}
		}
	}
	
	//TODO: combine converters
	
	return converters;
}

MultiImage MultiImage::optimized( Format format, QList<Image> originals, QList<Converter> converters, int base_image ){
	MultiImage multi( format, originals, converters );
	
	multi.steps.addBaseImage( base_image );
	multi.steps.findBestConverters();
	
	return multi;
}

MultiImage MultiImage::optimized( Format format, QList<Image> originals ){
	QList<Converter> converters = generateConverters( format, originals );
	
	//Only do the first one, unless high precision have been selected
	int test_amount = (format.get_precision() == 0) ? originals.size() : 1;
	QList<int> testing;
	for( int best_start=0; best_start<test_amount; best_start++ )
		testing << best_start;
	
	//Try all originals as the base image
	QList<MultiImage> imgs;
	{	ProgressBar progress( "Finding efficient solution", test_amount );
		imgs = map<MultiImage>( testing, [&](int i){
				progress.update();
				return optimized( format, originals, converters, i );
			} );
	}
	
	//Pick the one which causes the lowest file size
	MultiImage img( *maximum( imgs, [](MultiImage& img1, MultiImage& img2){
			return img1.compressed_size() < img2.compressed_size();
		} ) ); //TODO: access steps directly
	
	return img;
}

bool MultiImage::save( QString name ) const{
	//Parallel lists, 'converters' are the ones that produce 'primitives'
	QList<Image> primitives;
	QList<Converter const*> converters;
	
	//Extract all converters
	for( auto conversion : steps.getConversions() ){
		if( !converters.contains( conversion.getConverter() ) )
			converters << conversion.getConverter();
	}
	
	//Extract all primitives, and use 'converters' to keep track of them
	{	ProgressBar progress( "Optimizing images", converters.size() );
		for( auto converter : converters ){
			primitives << converter->get_primitive().auto_crop().optimize_filesize( format );
			progress.update();
		}
	}
	
	//Convert all converters to Frames, as needed by OraSaver
	QList<Frame> output;
	for( int i=0; i<originals.size(); i++ ){
		auto convs = steps.getConversionsTo( i );
		
		Frame f( primitives );
		for( auto conv : convs )
			f.layers << converters.indexOf( conv.getConverter() );
		output << f;
	}
	
	//TODO: OraSaver::save() should return bool
	OraSaver( primitives, output ).save( name + ".cgcompress", format );
	return true;
}

