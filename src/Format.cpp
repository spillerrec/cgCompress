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

#include "Format.hpp"
#include "FileSizeEval.hpp"
#include "formats/FormatWebP.hpp"

#include <QBuffer>
#include <QByteArray>
#include <QFile>

static QByteArray to_raw_data( QImage img ){
	img = img.convertToFormat( QImage::Format_ARGB32 );
	auto alpha = true; //Always including alpha actually seems to work better
	
	//Construct image
	auto pixel_size = alpha ? 4 : 3;
	QByteArray data( img.width()*img.height()*pixel_size, 0 );
	
	for( int iy=0; iy<img.height(); iy++ ){
		auto row = (const QRgb*)img.constScanLine( iy );
		for( int ix=0; ix<img.width(); ix++ ){
			auto offset = iy*img.width()*pixel_size + ix*pixel_size;
			data[offset + 0] = qRed(   row[ix] );
			data[offset + 1] = qGreen( row[ix] );
			data[offset + 2] = qBlue(  row[ix] );
			if( alpha )
				data[offset + 3] = qAlpha( row[ix] );
		}
	}
	
	return data;
}


/** Compress image to a memory buffer
 *  
 *  \param [in] img Image to save
 *  \return buffer containing the compressed image
 */
QByteArray Format::to_byte_array( QImage img ) const{
	if( format.toLower() == "raw" )
		return to_raw_data( img );
	
	QByteArray data;
	QBuffer buffer( &data );
	buffer.open( QIODevice::WriteOnly );
	if( format.toLower() == "webp" )
		FormatWebP::write( img, buffer, true, true );
	else
		img.save( &buffer, ext(), get_quality() );
	return data;
}

bool Format::save( QImage img, QString path ) const{
	if( format.toLower() == "raw" ){
		qWarning( "RAW mode should not be saved, only available as to_byte_array()" );
		return false;
	}
	if( format.toLower() == "webp" ){
		QFile file( filename(path) );
		if( !file.open( QIODevice::WriteOnly ) )
			return false;
		return FormatWebP::write( img, file, true, true );
	}
	
	return img.save( filename(path), ext(), get_quality() );
}

/** Estimate file size when compressed
 *  
 *  \param [in] img Image to estimate
 *  \param [in] p Precision of the estimation
 *  \return The estimated file size
 */
int Format::file_size( QImage img, Precision p ) const{
	if( precision_level > 0 && p != HIGH )
		return FileSize::image_gradient_sum( img );
	return to_byte_array( img ).size();
}

