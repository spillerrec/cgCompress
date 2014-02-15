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

#include <QDir>
#include <QFileInfo>
#include <QImageReader>

#include <qDebug>

/** Extracts the images in a cgcompress files to a directory with the same
 *  name. Directory must not exist beforehand.
 *  
 *  \todo Read the actual file instead of using the Qt image framework. This
 *  way we could use the names in the stack.xml for output names
 *  
 *  \param [in] filename File path to cgcompress file
 *  \param [in] format The format for the extracted files
 *  \return Return_Description
 */
void extract_cgcompress( QString filename, Format format ){
	QFileInfo file( filename );
	QDir current( file.dir() );
	if( !current.mkdir( file.baseName() ) ){
		qWarning( "Could not create directory for extracting" );
		return;
	}
	current.cd( file.baseName() );
	
	QImageReader reader( filename, "cgcompress" );
	
	if( !reader.canRead() ){
		if( !QImageReader::supportedImageFormats().contains( "cgcompress" ) )
			qWarning( "Extracting requires the cgCompress plug-in to be installed!" );
		else
			qWarning( "Could not read cgcompress file" );
		return;
	}
	
	QImage img;
	
	int i=0;
	while( reader.read( &img ) ){
		img.save( current.absolutePath() + "/" + QString::number( i++ ) + "." + format.ext(), format, format.get_quality() );
	}
}

void pack_directory( QString path ){
	
}
