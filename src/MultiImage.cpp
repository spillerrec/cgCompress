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

#include <algorithm>
#include <climits>
#include <iostream>
#include <string>

#include <QImageReader>
#include <QtConcurrent>
#include <QDebug>
#include <QElapsedTimer>


/** Find a converter to a frame not found
 *  \param [in,out] used_converters The converters found will be added to this
 *  \param [in] converters The available converters which may be used
 */
void add_converter( QList<Converter>& used_converters, const QList<Converter>& converters ){
	//Find out which have already been found
	QList<int> has;
	for( auto converter : used_converters )
		has << converter.get_to();
	
	//Find the best converter, from a found frame, to a not found frame
	const Converter* best_converter = nullptr;
	int filesize = INT_MAX;
	for( auto& converter : converters ){
		if( has.contains( converter.get_from() ) && !has.contains( converter.get_to() ) )
			if( converter.get_size() < filesize ){
				best_converter = &converter;
				filesize = converter.get_size();
			}
	}
	
	//Add the best converter
	if( !best_converter )
		qFatal( "No converter could be found!" );
	used_converters << *best_converter;
}

struct ConverterPara{
	const MultiImage* parent;
	int i, j;
	ConverterPara( const MultiImage* parent, int i, int j ) : parent(parent), i(i), j(j) { }
};
Converter createConverter( const ConverterPara& p ){
	return Converter( p.parent->originals, p.i, p.j, p.parent->format );
}

static void reuse_planes( QList<Image>& primitives, QList<Frame>& frames ){
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
				qDebug( "   Combining difference %d and %d", i, j );
				
				for( auto& frame : frames )
					for( auto& layer : frame.layers )
						if( layer == j )
							layer = i;
			}
		}
	}
}

static void reuse_planes2( QList<Image>& primitives, QList<Frame>& frames, Format format ){
	reuse_planes( primitives, frames );
	
	int amount_saved = 0;
	
	for( int i=0; i<primitives.size(); i++ ){
		if( !primitives[i].is_valid() )
			continue;
		
		//Try all possible combinations with later primitives, but only save those which can be shared
		QList<SplitImage> splits;
		for( int j=i+1; j<primitives.size(); j++ ){
			if( !primitives[j].is_valid() )
				continue;
			
			auto split = primitives[i].split_shared( primitives[j] );
			split.index = j;
			if( split.shared.auto_crop().is_valid() )
				splits << split;
		}
		
		
		if( splits.size() > 0 ){
			auto size_estim = [&](auto& img){ return img.auto_crop().compressed_size( format, Format::MEDIUM ); };
			auto size_exact = [&](auto& img){ return img.auto_crop().compressed_size( format, Format::HIGH   ); };
			
			//Calculate estimated file savings
			auto prim_i_size = size_estim( primitives[i] );
			for( int k=0; k<splits.size(); k++ ){
				//TODO: If it is used in multiple frames, our savings would increase
				auto& split = splits[k];
				auto new_size = size_estim( split.shared ) + size_estim( split.first ) + size_estim( split.second );
				auto old_size = prim_i_size + size_estim( primitives[split.index] );
				split.usefulness = old_size - new_size;
			}
			
			//Pick the best one
			auto best = std::max_element( splits.begin(), splits.end(), [](auto&a,auto&b){ return a.usefulness < b.usefulness; } );
			if( best != splits.end() ){
				//Do an exact evaluation of saved filesize
				auto normal_size = size_exact( primitives[i] ) + size_exact( primitives[best->index] );
				auto new_size = size_exact( best->shared ) + size_exact( best->first ) + size_exact( best->second );
				auto size_saved = normal_size - new_size;
				
				//Update if we save at least 128 bytes
				if( size_saved > 128 ){ //TODO: cutoff point is a constant
				//	qDebug( "   Extracting shared parts of difference %d and %d, saving %d bytes", i, best->index, size_saved );
					amount_saved += size_saved;
				
					//Change the primitives
					primitives[i          ] = Image( {}, {} );
					primitives[best->index] = Image( {}, {} );
					auto start_pos = primitives.size();
					primitives << best->shared;
					primitives << best->first;
					primitives << best->second;
					if( !best->shared.is_valid() || !best->first.is_valid() || !best->second.is_valid() )
						qFatal( "Not all splitted images are valid" );
					//TODO: Handle those cases
					
					//Update the frames with the new primitives
					for( auto& frame : frames ){
						frame.update_ids( i,           {start_pos+1, start_pos+0} );
						frame.update_ids( best->index, {start_pos+2, start_pos+0} );
						frame.primitives = primitives;
					}
					qDebug( "   Extracting shared parts of difference %d and %d, to {%d,%d} and {%d,%d}, saving %d bytes", i, best->index, start_pos+1, start_pos+0,start_pos+2,start_pos+0, size_saved );
				}
			}
			
		}
	}
	
	qDebug( "   Extracting saved %d bytes", amount_saved );
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
		for( int j=i+1; j<originals.size(); j++ ){
			converter_para.push_back( { this, i, j } );
			converter_para.push_back( { this, j, i } );
		}
	QElapsedTimer t;
	t.start();
	/*/
	QList<Converter> converters;
	for( int i=0; i<converter_para.count(); i++ )
		converters << createConverter( converter_para[i] );
	/*/
	auto future1 = QtConcurrent::mapped( converter_para, createConverter );
	ProgressBar::showFuture( "Generating data", future1 );
	auto converters = future1.results();
	//*/
	qDebug() << "Took:" << t.elapsed();
	
/*	for( auto converter : converters ){
		auto name = QString("converter_to_%1_from_%2_%3")
			.arg( QString::number(converter.get_to()  ), 3, QLatin1Char('0') )
			.arg( QString::number(converter.get_from()), 3, QLatin1Char('0') )
			.arg( converter.get_size() )
			;
		converter.get_primitive().auto_crop().save( name, {"webp"} );
	}//*/
	
	//Try all originals as the base image, and pick the best one
	int best_size = INT_MAX;
	QList<Image> final_primitives;
	QList<Frame> final_frames;
	
	//Only do the first one, unless high precision have been selected
	int test_amount = (format.get_precision() == 0) ? originals.size() : 1;
	{	ProgressBar progress( "Finding efficient solution", test_amount );
		for( int best_start=0; best_start<test_amount; best_start++, progress.update() ){
			QList<Converter> used_converters;
			used_converters << Converter( originals, best_start, best_start, format );
			
			for( int i=1; i<originals.size(); i++ )
				add_converter( used_converters, converters );
			
			qSort( used_converters.begin(), used_converters.end(), Converter::less_to );
			QList<Image> primitives;
			for( auto used : used_converters )
				primitives.append( used.get_primitive() );
			
			QList<Frame> frames;
			for( int i=0; i<originals.size(); i++ )
				frames << Frame( primitives, Converter::path( used_converters, i, best_start ) );
			
			//Evaluate file size and overwrite old solution if better
			int filesize = 0;
			if( test_amount > 0 ) //Skip this for the simple 1-test case
				for( auto& primitive : primitives )
					filesize += primitive.compressed_size( format, Format::MEDIUM );
			if( filesize < best_size ){
				best_size = filesize;
				final_primitives = primitives;
				final_frames = frames;
			}
		}
	}
	
	qDebug( "\nRevaluating differences (%d+)", final_primitives.size() );
	reuse_planes2( final_primitives, final_frames, format );
	
	auto future2 = QtConcurrent::map( final_primitives, [&]( auto& img ){ img = img.optimize_filesize( format ); } );
	ProgressBar::showFuture( "Optimizing final images", future2 );
	
	//TODO: Known not to work on transparent images
	//for( auto& frame : final_frames )
	//	frame.remove_pointless_layers();
	
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
		
		//*/
		for( int j=0; j<originals.count(); j++ ){
			auto path = QString("simitest/img %1 - %2.png")
				.arg( QString::number(i), 3, QLatin1Char('0') )
				.arg( QString::number(j), 3, QLatin1Char('0') )
				;
			similarities.getImagePart( i, j ).auto_crop().qimg().save( path );
		}
		/*/
		for( int j=0; j<originals.count(); j++ ){
			mask.combineMasks( similarities.getMask( j, i ) );
		}
		
		auto path = QString("simitest/mask %1")
				.arg( QString::number(i), 3, QLatin1Char('0') )
			;
		mask.apply( originals[i].qimg() ).auto_crop().save( path, {"webp"} );
		//*/
	}
	
	
//	QList<Image> final_primitives;
//	QList<Frame> final_frames;
	
//	OraSaver( final_primitives, final_frames ).save( name + ".cgcompress", format );
	return true;
}

/** Create an efficient composite version and save it to a cgCompress file.
 *  A faster version of method 1.
 *  \param [in] name File path for the output file, without the extension
 *  \return true on success
 */
bool MultiImage::optimize3( QString name ) const{
	if( originals.count() <= 0 )
		return true;
	
	//Part 
	QList<Converter> used_converters;
	QSet<int> all, done;
	for( int i=0; i<originals.size(); i++ )
		all << i;
	
	//Add the base image, always the first one (for now at least)
	int starting_image = 0;
	used_converters << Converter( originals, starting_image, starting_image, format );
	done << starting_image;
	
	//Add the remaining images
	for( int i=1; i<originals.size(); i++ ){
		//Create all needed converters
		QList<ConverterPara> converter_para;
		for( auto img_to : all.subtract( done ) )
			for( auto img_from : done )
				converter_para.push_back( { this, img_from, img_to } );
		auto converters = QtConcurrent::mapped( converter_para, createConverter ).results();
		//NOTE: this includes some from the previous iteration, however if we make the Converter more advanced, we can't reuse them
		
		//Find and add the best 
		auto best_it = std::min_element( converters.begin(), converters.end(), Converter::less_size );
		used_converters << *best_it;
		done << best_it->get_to();
	}
	
	//Fix the order
	qSort( used_converters.begin(), used_converters.end(), Converter::less_to );
	
	
	//Get all images for saving
	QList<Image> primitives;
	for( auto converter : used_converters )
		primitives << converter.get_primitive().auto_crop();
	
	//Get all paths from starting_image to each frame
	QList<Frame> frames;
	for( int i=0; i<originals.size(); i++ )
		frames << Frame( primitives, Converter::path( used_converters, i, starting_image ) );
	
	reuse_planes( primitives, frames );
	
	auto future2 = QtConcurrent::map( primitives, [&]( auto& img ){ img = img.optimize_filesize( format ); } );
	ProgressBar::showFuture( "Optimizing images", future2 );
	
	//Save cgCompress image
	OraSaver( primitives, frames ).save( name + ".cgcompress", format );
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
			qDebug( "Error found at image %d!", i+1 );
			img1.save( "error-decoded.png" );
			img2.save( "error-expected.png" );
			return false;
		}
	}
	
	//Fail if there are more images available
	return !reader.read( &current );
}

