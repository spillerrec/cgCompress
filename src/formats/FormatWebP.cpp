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
#include "../images/Rgba.hpp"
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


RgbaImage FormatWebP::readRgbaImage( QByteArray data ){
	int width, height;
	
	uint8_t *raw = WebPDecodeBGRA( (uint8_t*)data.data(), data.size(), &width, &height );
	if( !raw )
		return { 0,0 };
	
	return RgbaImage( reinterpret_cast<Rgba*>( raw ), width, height );
}


class Compressor{
	private:
		WebPConfig config;
		WebPPicture pic;
		WebPMemoryWriter writer;
		std::unique_ptr<uint8_t[]> data;
		bool validate(){ return WebPValidateConfig( &config ); }
		
	public:
		Compressor(){ writer.mem = nullptr; }
		~Compressor(){ free( writer.mem ); }
		
		bool init( QImage );
		bool init( ConstRgbaView );
		
		void setupLossless( bool keep_alpha, int quality=100 );
		void setupLossy( int quality );
		
		bool write( QIODevice& );
		int fileSize();
};
bool Compressor::init( QImage image ){
	unsigned stride = 4 * image.width();
	data = std::make_unique<uint8_t[]>( stride * image.height() );
	
	//Make sure the input is in ARGB
	if( image.format() != QImage::Format_RGB32 && image.format() != QImage::Format_ARGB32 )
		image = image.convertToFormat( QImage::Format_ARGB32 );
	
	for( int iy=0; iy<image.height(); ++iy ){
		const QRgb* in = (const QRgb*)image.constScanLine( iy );
		uint8_t* out = data.get() + iy*stride;
		
		for( int ix=0; ix<image.width(); ++ix, ++in ){
			*(out++) = qBlue(  *in );
			*(out++) = qGreen( *in );
			*(out++) = qRed(   *in );
			*(out++) = qAlpha( *in );
		}
	}
	
	//Initialize structures
	if( !WebPPictureInit( &pic ) || !WebPConfigInit( &config ) )
		return false;
	
	pic.width  = image.width();
	pic.height = image.height();
	pic.argb_stride = stride;
	pic.use_argb = true;
	pic.argb = (uint32_t*)data.get();
	pic.argb_stride = image.width();
	
	return true;
}
bool Compressor::init( ConstRgbaView image ){
	//Initialize structures
	if( !WebPPictureInit( &pic ) || !WebPConfigInit( &config ) )
		return false;
	
	pic.width  = image.width();
	pic.height = image.height();
	pic.argb_stride = image.stride() * 4;
	pic.use_argb = true;
	pic.argb = const_cast<uint32_t*>(reinterpret_cast<const uint32_t*>(image.rawData()));
	pic.argb_stride = image.width();
	
	return true;
}

void Compressor::setupLossless( bool keep_alpha, int quality ){
	config.lossless = 1;
	config.exact = keep_alpha ? 1 : 0;
	config.quality = quality;
	config.method = 6*quality / 100;
}
void Compressor::setupLossy( int quality ){
	//TODO:
}
bool Compressor::write( QIODevice& device ){
	if( !validate() )
		return false;
	
	WebPMemoryWriterInit(&writer);
	pic.writer = WebPMemoryWrite;
	pic.custom_ptr = &writer;
	
	if( !WebPEncode(&config, &pic) ){
		qDebug( "WebP encode failed with %d", pic.error_code );
		return false;
	}
	
	device.write( (char*)writer.mem, writer.size ); //TODO: Check?
	return true;
}

int Compressor::fileSize(){
	if( !validate() )
		return -1;
	
	//TODO: Do something smarter than creating an array
	WebPMemoryWriterInit(&writer);
	pic.writer = WebPMemoryWrite;
	pic.custom_ptr = &writer;
	
	if( !WebPEncode(&config, &pic) ){
		qDebug( "WebP encode failed with %d", pic.error_code );
		return -1;
	}
	
	return writer.size;
}

bool FormatWebP::write( QImage image, QIODevice& device, bool keep_alpha, int quality ){
	Compressor webp;
	if( !webp.init( image ) )
		return false;
	
	webp.setupLossless( keep_alpha, quality );
	
	return webp.write( device );
}

bool FormatWebP::write( ConstRgbaView image, QIODevice& device, bool keep_alpha, int quality ){
	Compressor webp;
	if( !webp.init( image ) )
		return false;
	
	webp.setupLossless( keep_alpha, quality );
	
	return webp.write( device );
}

bool FormatWebP::writeLossy( QImage image, QIODevice& device, int quality ){
	return image.save( &device, "webp", quality ); //TODO: Quickfix
	Compressor webp;
	if( !webp.init( image ) )
		return false;
	
	webp.setupLossy( quality );
	
	return webp.write( device );
}


int FormatWebP::estimate_filesize( ConstRgbaView image, bool keep_alpha ){
	Compressor webp;
	if( !webp.init( image ) )
		return false;
	
	webp.setupLossless( keep_alpha, 0 );
	
	return webp.fileSize();
}
