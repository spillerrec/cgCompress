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

#include <QFileInfo>
#include <QImageReader>
#include <QFile>

#include <qDebug>

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
	while( reader.read( &img ) )
		files.append( { QString::number( i++ ), img } );
	
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
	
	for( auto file : extract_files( filename ) )
		file.second.save( current.absolutePath() + "/" + file.first + "." + format.ext(), format, format.get_quality() );
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
