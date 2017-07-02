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
#include "ProgressBar.hpp"

#include "ImageSimilarities.hpp"

#include <climits>
#include <iostream>
#include <string>

#include <QImageReader>
#include <QtConcurrent>
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

/** Find a converter to a frame not found, will be called recursively to find
 *  all frames.
 *  \param [in,out] used_converters The converters found will be added to this
 *  \param [in,out] frames The list of transformations to get the images. Each
 *                     transformation contains a the indexes of images to get
 *                     to the final image.
 *  \param [in] converters The available converters which may be used
 *  \param [in] amount The total amount of frames
 */
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

struct ConverterPara{
	const MultiImage* parent;
	int i, j;
	ConverterPara( const MultiImage* parent, int i, int j ) : parent(parent), i(i), j(j) { }
};
Converter createConverter( const ConverterPara& p ){
	return Converter( p.parent->originals, p.i, p.j, p.parent->format );
}

template<typename T>
void showProgress( const char* description, QFuture<T>& future ){
	ProgressBar progress( description, future.progressMaximum() - future.progressMinimum() );
	
	int last = future.progressMinimum();
	while( !future.isFinished() ){
		int current = future.progressValue();
		if( current > last ){
			progress.update( current - last );
			last = current;
		}
		QThread::msleep( 10 );
	}
	
	future.waitForFinished();
}

/** Create an efficient composite version and save it to a cgCompress file.
 *  \param [in] name File path for the output file, without the extension
 *  \return true on success
 */
bool MultiImage::optimize( QString name ) const{
	if( originals.count() <= 0 )
		return true;
	
	QList<ConverterPara> converter_para;
	for( int i=0; i<originals.size(); i++ )
		for( int j=i+1; j<originals.size(); j++ )
			converter_para.push_back( { this, i, j } );
	auto future1 = QtConcurrent::mapped( converter_para, createConverter );
	showProgress( "Generating data", future1 );
	auto converters = future1.results();
	
	//Try all originals as the base image, and pick the best one
	int best_size = INT_MAX;
	QList<Image> final_primitives;
	QList<Frame> final_frames;
	
	//Only do the first one, unless high precision have been selected
	int test_amount = (format.get_precision() == 0) ? originals.size() : 1;
	{	ProgressBar progress( "Finding efficient solution", test_amount );
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
					int from = (j == 1) ? best_start : comb[j-1];
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
	
	auto future2 = QtConcurrent::map( final_primitives, [&]( auto& img ){ img = img.optimize_filesize( format ); } );
	showProgress( "Optimizing images", future2 );
	
	OraSaver( final_primitives, final_frames ).save( name + ".cgcompress", format );
	return true;
}

/** Alternative version of ::optimize()
 *  \param [in] name File path for the output file, without the extension
 *  \return true on success
 */
bool MultiImage::optimize2( QString name ) const{
	if( originals.count() <= 0 )
		return true;
	
	QList<Converter> converters;
	ImageSimilarities similarities;
	{	ProgressBar progress( "Finding similarities", originals.size() );
		for( int i=0; i<originals.size(); i++ ){
			similarities.addImage( originals[i].qimg() );
			progress.update();
			//TODO: each call is not the same complexity!
		}
	}
	
	/*
	for( int i=0; i<originals.count(); i++ )
		for( int j=0; j<=i; j++ ){
			auto path = QString("simitest/img %1 - %2.png")
				.arg( QString::number(i), 3, QLatin1Char('0') )
				.arg( QString::number(j), 3, QLatin1Char('0') )
				;
			qDebug( path.toLocal8Bit().constData() );
			similarities.getImagePart( i, j ).save( path );
		}
	*/
	
	for( int i=0; i<originals.count(); i++ ){
		ImageMask mask( originals[i].qimg().size() );
		mask.fill( 0 );
		
		for( int j=0; j<originals.count(); j++ ){
			mask.combineMasks( similarities.getMask( j, i ) );
		}
		
		auto path = QString("simitest/mask %1.png")
				.arg( QString::number(i), 3, QLatin1Char('0') )
			;
		mask.apply( originals[i].qimg() ).save( path );
	}
	
	
//	QList<Image> final_primitives;
//	QList<Frame> final_frames;
	
//	OraSaver( final_primitives, final_frames ).save( name + ".cgcompress", format );
	return true;
}

/** \return True if 'file' is decoded exactly like this MultiImage
 *  \param [in] file File path for file to validate
 */
bool MultiImage::validate( QString file ) const{
	QImageReader reader( file );
	
	QImage current;
	for( int i=0; i<originals.count(); i++ ){
		if( !reader.read( &current ) )
			return false;
		
		auto img1 = current            .convertToFormat( QImage::Format_ARGB32 );
		auto img2 = originals[i].qimg().convertToFormat( QImage::Format_ARGB32 );
		
		if( img1 != img2 ){
			img1.save( "error-decoded.png" );
			img2.save( "error-expected.png" );
			return false;
		}
	}
	
	//Fail if there are more images available
	return !reader.read( &current );
}

