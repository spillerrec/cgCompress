/*
	This file is part of qt5-webp-plugin.

	qt5-webp-plugin is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	qt5-webp-plugin is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with qt5-webp-plugin.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QImage>
#include <QByteArray>
#include <QImageIOHandler>
#include <QImageIOPlugin>
#include <QColor>
#include <QVariant>

#include <memory>


#include "FormatWebP.hpp"
#include "webp/decode.h"
#include "webp/encode.h"

QImage FormatWebP::read( QByteArray data ){
	int width, height;
	
	uint8_t *raw = WebPDecodeRGBA( (uint8_t*)data.data(), data.size(), &width, &height );
	if( !raw )
		return { };
	
	QImage img( width, height, QImage::Format_ARGB32 );
	for( int iy=0; iy<height; iy++ ){
		QRgb* row = (QRgb*)img.scanLine( iy );
		
		for( int ix=0; ix<width; ix++ ){
			uint8_t *pixel = raw + ( iy*width + ix ) * 4;
			row[ix] = qRgba( pixel[0], pixel[1], pixel[2], pixel[3] );
		}
	}
	
	free( raw );
	return img;
}

bool FormatWebP::write( QImage image, QIODevice& device, bool keep_alpha, bool high_compression ){
	//bool alpha = image.hasAlphaChannel();
	//TODO: Delete contents of transparent pixels when !keep_alpha && alpha
	unsigned stride = 4 * image.width();
	
	auto data = std::make_unique<uint8_t>( stride * image.height() );
	if( !data )
		return false;
	
	//Make sure the input is in ARGB
	if( image.format() != QImage::Format_RGB32 && image.format() != QImage::Format_ARGB32 )
		image = image.convertToFormat( QImage::Format_ARGB32 );
	
	for( int iy=0; iy<image.height(); ++iy ){
		const QRgb* in = (const QRgb*)image.constScanLine( iy );
		uint8_t* out = data.get() + iy*stride;
		
		for( int ix=0; ix<image.width(); ++ix, ++in ){
			*(out++) = qRed(   *in );
			*(out++) = qGreen( *in );
			*(out++) = qBlue(  *in );
			*(out++) = qAlpha( *in );
		}
	}
	
	WebPConfig config;
	config.lossless = 1;
	config.quality = 100;
	config.method = high_compression ? 6 : 0;
	config.image_hint = WEBP_HINT_GRAPH; //TODO: compare
	//config.image_hint = WEBP_HINT_PICTURE;
	//config.image_hint = WEBP_HINT_DEFAULT;
	if( !WebPValidateConfig( &config ) )
		return false;
	
	WebPPicture pic;
	if( !WebPPictureInit( &pic ) )
		return false;
	pic.use_argb = true;
	pic.argb = (uint32_t*)data.get();
	pic.argb_stride = image.width();
	
	// Set up a byte-writing method (write-to-memory, in this case):
	WebPMemoryWriter writer;
	WebPMemoryWriterInit(&writer);
	pic.writer = WebPMemoryWrite;
	pic.custom_ptr = &writer;
	
	if( !WebPEncode(&config, &pic) )
		return false;
	
	device.write( (char*)writer.mem, writer.size );
	free( writer.mem ); //TODO: Wrong if device throws?
	
	return true;
}
