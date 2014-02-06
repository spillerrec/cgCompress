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

#include <QFileInfo>
#include <QDateTime>

OraSaver::OraSaver( QList<Image> images ) : primitives( images ){
	Frame frame( primitives );
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

bool addFileInfo( zipFile &zf, QFileInfo file ){
	zip_fileinfo zi;

	zi.dosDate = 0;
	zi.internal_fa = 0;
	zi.external_fa = 0;
	
	QDateTime time = file.lastModified().toTimeSpec( Qt::UTC );
	setTime( zi.tmz_date, time.date(), time.time() );
	
	int compression = ( true ) ? 0 : Z_DEFLATED;
	int compression_level = ( true ) ? 0 : Z_DEFAULT_COMPRESSION;
	
	//Start file
	int err = zipOpenNewFileInZip3_64(
			zf, file.fileName().toLocal8Bit().constData(), &zi
		,	NULL, 0, NULL, 0, NULL // comment
		,	compression, compression_level, 0
		,	-MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY
		,	NULL, 0, 0  // password, crcFile, zip64);
		);
	
	
	qDebug( "Size of %s ", file.filePath().toLocal8Bit().constData() );
	//Read file
	QFile reader( file.filePath() );
	if( !reader.open( QIODevice::ReadOnly ) )
		return false; //close zipOpen... ?
	QByteArray data = reader.readAll();
	
	qDebug( "Size of %s is %d", file.filePath().toLocal8Bit().constData(), data.size() );
	
	//Write file
	zipWriteInFileInZip( zf, data.constData(), data.size() );
	
	//Finish file
	reader.close();
	return zipCloseFileInZip( zf ) == ZIP_OK;;
}

void OraSaver::save( QString path, const char* format ) const{
	if( frames.isEmpty() ){
		qDebug( "OraSaver: no frames to save!" );
		return;
	}
	
	//Open zip
	zipFile zf = zipOpen64( path.toLocal8Bit().constData(), 0 );
	
	//TODO: save mimetype
	addStringFile( zf, "mimetype", QString( "image/openraster" ) );
	
	//* Thumbnail
	Image thumb = frames.first().reconstruct().resize( 256 );
	addByteArray( zf, "Thumbnails/thumbnail.png", thumb.to_byte_array( "png" ) );
	//*/
	
	//Find used primitives
	QList<int> used;
	for( auto frame : frames )
		for( auto layer : frame.layers )
			if( !used.contains( layer ) )
				used.append( layer );
	
	for( auto layer : used ){
		QString name = QString( "data/%1.%2" ).arg( layer ).arg( format );
		addByteArray( zf, name, primitives[layer].to_byte_array( format ) );
	}
	
	//TODO: stack
	QString stack( "<?xml version='1.0' encoding='UTF-8'?>\n" );
	stack += QString( "<image w=\"%1\" h=\"%2\">" ).arg( primitives[0].qimg().width() ).arg( primitives[0].qimg().height() );
	
	//	stack += "<stack>";
	for( auto frame : frames ){
		stack += "<stack>";
		
		for( auto layer : boost::adaptors::reverse(frame.layers) ){
			QString name = QString( "data/%1.%2" ).arg( layer ).arg( format );
			stack += QString( "<layer name=\"%1\" src=\"%1\" x=\"%2\" y=\"%3\" />" )
				.arg( name ).arg( primitives[layer].get_pos().x() ).arg( primitives[layer].get_pos().y() );
		}
		
		stack += "</stack>";
	}
	//	stack += "</stack>";
	
	stack += "</image>";
	
	//TODO: compress this
	addStringFile( zf, "stack.xml", stack, true );
	
	zipClose( zf, NULL );
}
