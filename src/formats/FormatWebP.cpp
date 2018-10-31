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
#include <memory>


#include "FormatWebP.hpp"
#include "../images/Rgba.hpp"
#include "webp/decode.h"
#include "webp/encode.h"


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
		
		bool init( ConstRgbaView );
		
		void setupLossless( bool keep_alpha, int quality=100 );
		void setupLossy( int quality );
		
		bool write( QIODevice& );
		int fileSize();
};

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

bool FormatWebP::write( ConstRgbaView image, QIODevice& device, bool keep_alpha, int quality ){
	Compressor webp;
	if( !webp.init( image ) )
		return false;
	
	webp.setupLossless( keep_alpha, quality );
	
	return webp.write( device );
}

bool FormatWebP::writeLossy( ConstRgbaView image, QIODevice& device, int quality ){
	return toQImage(image).save( &device, "webp", quality ); //TODO: Quickfix
	
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
