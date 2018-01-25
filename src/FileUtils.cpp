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

#include "FileUtils.hpp"

#include <QBuffer>
#include <QFileInfo>
#include <QImageReader>
#include <QFile>

#include <QDebug>

#include "Format.hpp"
#include "Compression.hpp"
#include "CsvWriter.hpp"
#include "OraSaver.hpp"

/** Extracts the images in a cgCompress file.
 *  
 *  \todo Read the actual file instead of using the Qt image framework. This
 *  way we could use the names in the stack.xml for output names
 *  
 *  \param [in] filename File path to cgCompress file
 *  \return The images and the names of the images
 */
QList<std::pair<QString,QImage>> extract_files( QString filename ){
	QList<std::pair<QString,QImage>> files;
	QImageReader reader( filename, "cgcompress" );
	
	if( !reader.canRead() ){
		if( !QImageReader::supportedImageFormats().contains( "cgcompress" ) )
			qWarning( "Extracting requires the cgCompress plug-in to be installed!" );
		else
			qWarning( "Could not read cgCompress file" );
		return files;
	}
	
	QImage img;
	
	int i=0;
	while( reader.read( &img ) ){
		auto index = QString( "%1" ).arg( i++, 4, 10, QChar{'0'} );
		files.append( { index, img } );
	}
	
	return files;
}

/** Extracts the images in a cgCompress file to a directory with the same
 *  name. Directory must not exist beforehand.
 *  
 *  \param [in] filename File path to cgCompress file
 *  \param [in] format The format for the extracted files
 */
void extract_cgcompress( QString filename, Format format ){
	QFileInfo file( filename );
	QDir current( file.dir() );
	if( !current.mkdir( file.baseName() ) ){
		qWarning( "Could not create directory for extracting" );
		return;
	}
	current.cd( file.baseName() );
	
	for( auto file2 : extract_files( filename ) )
		format.save( file2.second, current.absolutePath() + "/" + file.baseName() + file2.first + "." + file.suffix() );
}

void evaluate_cgcompress( QStringList files ){
	CsvWriter csv( "evaluatation.csv", {"File", "Image count", "BMP size", "LZMA size", "LZ4 size", "cgCompress size", "PNG size", "WebP size"} );
	
	for( auto file : files ){
		qDebug( "Evaluating %s", file.toLocal8Bit().constData() );
		auto images = extract_files( file );
		if( images.size() == 0 ){
			qDebug( "Failed to extract file:  %s", file.toLocal8Bit().constData() );
			continue;
		}
		
		//File name
		csv.write( file );
		
		csv.write( images.size() );
		
		//BMP size
		QBuffer data;
		data.open( QBuffer::ReadWrite );
		for( auto image : images )
			data.write( Format( "bmp" ).to_byte_array( image.second ) );
		csv.write( int(data.size()) );
		
		//LZMA compressed
		csv.write( FileSize::lzma_compress_size( reinterpret_cast<const unsigned char*>(data.data().constData()), data.size() ) );
		
		//LZ4 compression
		csv.write( FileSize::lz4compress_size( reinterpret_cast<const unsigned char*>(data.data().constData()), data.size() ) );
		
		//cgCompress size
		csv.write( int(QFileInfo( file ).size()) );
		
		//PNG file size
		uint64_t png_size = 0;
		for( auto image : images )
			png_size += Format( "bmp" ).to_byte_array( image.second ).size();
		csv.write( int(png_size) );
		
		//WebP file size
		uint64_t webp_size = 0;
		for( auto image : images )
			webp_size += Format( "webp" ).to_byte_array( image.second ).size();
		csv.write( int(webp_size) );
		
		csv.stop();
	}
}

/** Add all files in a sub-directory to files. File name will be "sub_dir/filename".
 *  
 *  \param [in,out] files All files will be added in this list
 *  \param [in] dir Root directory
 *  \param [in] sub_dir Sub-directory containing the wanted files
 */
static void append_all_files( QList<std::pair<QString,QByteArray>>& files, QDir dir, QString sub_dir ){
	auto data = QDir( dir.absolutePath() + "/" + sub_dir ).entryInfoList( QStringList() << "*.*", QDir::Files );
	for( auto file : data ){
		QFile file_data( file.absoluteFilePath() );
		if( file_data.open( QIODevice::ReadOnly ) )
			files.append( { sub_dir + "/" + file.fileName(), file_data.readAll() });
		else
			qWarning() << "Could not read file!" << file.filePath();
	}
}

/** Re-zip an unpacked OpenRaster zip archive
 *  
 *  \param [in] dir Path to the directory
 */
void pack_directory( QDir dir ){
	if( !dir.exists() ){
		qWarning( "Path must be a directory!" );
		return;
	}
	
	//Get mimetype
	QFile mime( dir.absolutePath() + "/mimetype" );
	if( !mime.open( QIODevice::ReadOnly ) ){
		qWarning( "mimetype missing" );
		return;
	}
	QString mimetype = mime.readAll();
	
	//Get stack
	QFile stack_file( dir.absolutePath() + "/stack.xml" );
	if( !stack_file.open( QIODevice::ReadOnly ) ){
		qWarning( "stack.xml missing" );
		return;
	}
	QString stack = stack_file.readAll();
	
	//Get thumbnail
	QList<std::pair<QString,QByteArray>> files;
	append_all_files( files, dir, "Thumbnails" );
	
	//Get data files
	append_all_files( files, dir, "data" );
	
	OraSaver::save( dir.dirName() + ".packed.cgcompress", mimetype, stack, files );
}

bool isSimilar( QImage img1, QImage img2 ){
	if( img1.size() != img2.size() )
		return false;
	
	int count=0, total=0;
	for( int iy=0; iy<img1.height(); iy++ )
		for( int ix=0; ix<img1.width(); ix++ ){
			auto pix1 = img1.pixel( ix, iy );
			auto pix2 = img2.pixel( ix, iy );
			if( qAlpha( pix1 ) != 0 || qAlpha( pix2 ) != 0 ){
				total++;
				if( pix1 == pix2 )
					count++;
			}
		}
	
	if( total == 0 )
		return false;
	
	double ratio = (double)count / total;
	if( ratio >= 0.05 ) //More than 5% of all pixels can be reused
		return true;
	
	return false;
}

QStringList expandFolders( QStringList paths ){
	QStringList files;
	
	for( auto path : paths )
		if( QFileInfo( path ).isDir() ){
			for( auto file : QDir( path ).entryList( { "*" }, QDir::Files ) )
				files << path + "/" + file;
		}
		else
			files << path;
	
	return files;
}


/** \return This image where all transparent pixels are set to discard_color */
QImage discardTransparent( QImage img, QRgb discard_color ){
	QImage output( img.convertToFormat(QImage::Format_ARGB32) );
	
	auto size = output.size();
	for( int iy=0; iy<size.height(); iy++ ){
		QRgb* out = (QRgb*)output.scanLine( iy );
		for( int ix=0; ix<size.width(); ix++ )
			if( qAlpha( out[ix] ) == 0 )
				out[ix] = discard_color;
	}
			
	return output;
}

QImage withoutAlpha( QImage img ){
	return img.convertToFormat(QImage::Format_RGB32).convertToFormat(QImage::Format_ARGB32);
}

