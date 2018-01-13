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

#include "OraSaver.hpp"
#include "ProgressBar.hpp"

#include <QFileInfo>
#include <QDateTime>

/** Construct an unoptimized cgCompress file from a set of images.
 *  
 *  \param [in] images The images to form individual frames
 */
OraSaver::OraSaver( QList<Image> images ) : primitives( images ){
	Frame frame( primitives, {} );
	for( int i=0; i<images.size(); i++ )
		frame.layers.append( i );
	frames.append( frame );
}


#include "minizip/zip.h"
#include <boost/range/adaptor/reversed.hpp>

void setTime( tm_zip &tm, QDate date, QTime time ){
	tm.tm_sec  = time.second();
	tm.tm_min  = time.minute();
	tm.tm_hour = time.hour();
	tm.tm_mday = date.day();
	tm.tm_mon  = date.month();
	tm.tm_year = date.year();
}

bool addByteArray( zipFile &zf, QString name, QByteArray arr, int compression=0 ){
	zip_fileinfo zi;

	zi.dosDate = 0;
	zi.internal_fa = 0;
	zi.external_fa = 0;
	
	QDateTime time = QDateTime::currentDateTimeUtc();
	setTime( zi.tmz_date, time.date(), time.time() );

	//Start file
	int err = zipOpenNewFileInZip3_64(
			zf, name.toUtf8().constData(), &zi
		,	NULL, 0, NULL, 0, NULL // comment
		, (compression != 0) ? Z_DEFLATED : 0, compression, 0
		//,	0, 0, 0
		,	-MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY
		,	NULL, 0, 0  // password, crcFile, zip64);
		);
	
	//Write file
	zipWriteInFileInZip( zf, arr.constData(), arr.size() );
	
	//Finish file
	return zipCloseFileInZip( zf ) == ZIP_OK;;
}

bool addStringFile( zipFile &zf, QString name, QString contents, bool compress=false ){
	return addByteArray( zf, name, contents.toUtf8(), compress ? 9 : 0 );
}

/** Saves a zip compressed archive in the OpenRaster style.
 *  
 *  \param [in] path File path for output file
 *  \param [in] mimetype The contents of "mimetype" which will be STORED
 *  \param [in] stack The contents of "stack.xml"
 *  \param [in] files File names and contents of the files
 */
void OraSaver::save( QString path, QString mimetype, QString stack, QList<std::pair<QString,QByteArray>> files ){
	//TODO: make wrapper class for zipFile
	zipFile zf = zipOpen64( path.toLocal8Bit().constData(), 0 );
	
	//Save mimetype without compression
	addStringFile( zf, "mimetype", mimetype );
	
	//Save stack with compression
	addStringFile( zf, "stack.xml", stack, true );
	
	//Save all data files
	for( auto file : files )
		addByteArray( zf, file.first, file.second );
		//TODO: compress if there are significant savings. Perhaps user defined threshold?
	
	zipClose( zf, NULL );
}

/** Save the current frames as a cgCompress file.
 *  
 *  \param [in] path File path for output file
 *  \param [in] format Format for compressing the image files
 */
void OraSaver::save( QString path, Format format ) const{
	if( frames.isEmpty() ){
		qWarning( "OraSaver: no frames to save!" );
		return;
	}
	
	auto first_frame = frames.first().reconstruct();
	
	QList<std::pair<QString,QByteArray>> files;
	
	// Thumbnail
	Image thumb = first_frame.resize( 256 );
	Format lossy = format.get_lossy();
	files.append( { lossy.filename("Thumbnails/thumbnail"), thumb.to_byte_array( lossy ) } );
	
	//Find used primitives
	QList<int> used;
	for( auto frame : frames )
		for( auto layer : frame.layers )
			if( !used.contains( layer ) )
				used.append( layer );
	
	{	ProgressBar progress( "Saving", used.count() );
		for( auto layer : used ){
			QString name = QString( "data/%1.%2" ).arg( layer ).arg( format.ext() );
			files.append( { name, primitives[layer].to_byte_array( format ) } );
			progress.update();
		}
	}
	
	//Create stack
	QString stack( "<?xml version='1.0' encoding='UTF-8'?>\n" );
	stack += QString( "<image w=\"%1\" h=\"%2\">" ).arg( first_frame.qimg().width() ).arg( first_frame.qimg().height() );
	
	for( auto frame : frames ){
		stack += "<stack>";
		
		for( auto layer : boost::adaptors::reverse(frame.layers) ){
			//Detect composition mode
			//TODO: Decide this earlier and change encoding type
			QString composition = "composite-op=\"cgcompress:alpha-replace\"";
		//	if( !primitives[layer].mustKeepAlpha() )
		//		composition = "";
			
			QString name = QString( "data/%1.%2" ).arg( layer ).arg( format.ext() );
			stack += QString( "<layer %1 name=\"%2\" src=\"%2\" x=\"%3\" y=\"%4\" />" )
				.arg( composition )
				.arg( name )
				.arg( primitives[layer].get_pos().x() )
				.arg( primitives[layer].get_pos().y() )
				;
		}
		
		stack += "</stack>";
	}
	
	stack += "</image>";
	
	//Save zip archive
	save( path, "image/openraster", stack, files );
}
