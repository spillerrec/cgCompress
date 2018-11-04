/*
	This file is part of qt5-ora-plugin.

	qt5-ora-plugin is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	qt5-ora-plugin is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with qt5-ora-plugin.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "OraHandler.hpp"
#include "../formats/FormatWebP.hpp"
#include "../images/Blend.hpp"
#include "CgImage.hpp"

#include <cstring>
#include <iterator>
#include <iostream>

#include <QFile>
#include <QFileInfo>
#include <QBuffer>
#include <QVariant>

#include <pugixml.hpp>
#include <archive.h>
#include <archive_entry.h>


struct ReadingData{
	QIODevice& device;
	qint64 buff_size; //TODO: how large should this be?
	QByteArray arr;
	
	ReadingData( QIODevice& device, int buff_size = 1024*4 ) : device( device ), buff_size(buff_size) { }
};
ssize_t stream_read( archive*, void* data, const void** buff ){
	ReadingData& stream = *(ReadingData*)data;
	stream.arr = stream.device.read( stream.buff_size );
	*buff = stream.arr.constData();
	return stream.arr.size();
}

int stream_close( archive*, void *data ){
	((ReadingData*)data)->device.close();
	return ARCHIVE_OK;
}

class Archive{
	private:
		archive* a {nullptr};
		ReadingData data;
		
		
	public:
		Archive( QIODevice& device ) : a( archive_read_new() ), data(device){
			archive_read_support_format_zip(a);
			
			if( archive_read_open( a, &data, nullptr, stream_read, stream_close ) )
				throw std::runtime_error( "couldn't open" + std::string( archive_error_string(a) ) );
		}
		
		~Archive(){
			archive_read_close( a );
			archive_read_free(  a );
		}
		
		QByteArray read_data(){
			QByteArray raw;
			
			//Read chuncks
			const char *buff;
			size_t size;
			int64_t offset;
			
			while( true ){
				switch( archive_read_data_block( a, (const void**)&buff, &size, &offset ) ){
					case ARCHIVE_OK: raw += QByteArray( buff, size ); break;
					case ARCHIVE_EOF: return raw;
					default:
						qWarning( "Error while reading zip data: %s", archive_error_string(a) );
						return raw;
				}
			}
		}
		
		QString next_file(){
			archive_entry *entry;
			switch( archive_read_next_header( a, &entry ) ){
				case ARCHIVE_EOF: return QString();
				case ARCHIVE_OK:
					return QString::fromWCharArray( archive_entry_pathname_w( entry ) );
					
				default:
					qWarning( "Can't read the next zip header: %s", archive_error_string(a) );
					return QString();
			}
		}
};

void OraDecoder::alpha_replace( RgbaView output, ConstRgbaView image, int dx, int dy ){
	auto new_width  = std::min(image.width() , output.width()-dx);
	auto new_height = std::min(image.height(), output.width()-dy);
	//TODO: Warning if inconsistent?
	auto crop = output.crop( dx, dy, new_width, new_height );
	Blending::BlendImages<Rgba>( crop, image.crop(0,0,new_width,new_height), Blending::alphaReplace );
}

void OraDecoder::src_over( RgbaView output, ConstRgbaView image, int dx, int dy ){
	auto new_width  = std::min(image.width() , output.width()-dx);
	auto new_height = std::min(image.height(), output.width()-dy);
	auto crop = output.crop( dx, dy, new_width, new_height );
	Blending::BlendImages<Rgba>( crop, image.crop(0,0,new_width,new_height), Blending::srcOverRgba );
}


CgBlendType blendTypeFromString( const std::string& str ){
	if( str == "cgcompress:alpha-replace" )
		return CgBlendType::ALPHA_REPLACE;
	else if( str == "svg:src-over" || str == "" )
		return CgBlendType::SRC_OVER;
	else
		throw std::runtime_error( "Unsupported composite-op: " + str );
}


CgImage loadCgImage( QIODevice& device ){
	CgImage image;
	Archive a(device);
	
	bool stack_loaded = false;
	pugi::xml_document doc;
	
	//Read and check mime
	QString mime = a.next_file();
	if( mime.isNull() )
		throw std::runtime_error( "CgImage is not an usable zip archive" );
		
	if( mime != "mimetype" )
		throw std::runtime_error( "CgImage must start with the mimetype" );
	//TODO: check if it is STORED?
	
	QByteArray mime_data = a.read_data();
	if( !(mime_data == "image/openraster" || mime_data == "image/x-cgcompress") )
		throw std::runtime_error( "Mimetype does not match" );
	
	//Read all files
	QString name;
	while( !(name = a.next_file()).isNull() ){
		if( name.startsWith( "data/" ) ){
			QString suffix = QFileInfo(name).suffix();
			CgPrimitive pri;
			pri.img =  FormatWebP::readRgbaImage( a.read_data() );
			pri.filename = name.toUtf8().constData();
			
			if( pri.img.width() != 0 )
				image.addPrimitive( std::move(pri) );
			else
				std::cout << "Could not read data file: " << pri.filename.c_str() << '\n';
		}
		else if( name == "stack.xml" ){
			QByteArray data = a.read_data();
			if( !doc.load_buffer( data.data(), data.size() ) )
				throw std::runtime_error( "Could not parse stack.xml" );
			stack_loaded = true;
		}
		else
			std::cout << "Unknown file: " << name.toLocal8Bit().constData() << '\n';
		//TODO: Thumbnail
	}
	
	if( !stack_loaded )
		throw std::runtime_error( "stack.xml not found" );
	
	//Parse stack.xml
	auto img = doc.child( "image" );
	int width = img.attribute( "w" ).as_int( -1 );
	int height = img.attribute( "h" ).as_int( -1 );
	image.setDimensions( width, height );
	
	auto stack = img.child( "stack" );
	if( !stack )
		throw std::runtime_error( "No frames found" );
	do{
		CgFrame frame;
		
		for( auto it = --stack.end(); it != --stack.begin(); it-- )
			if( it->name() == std::string("layer") ){
				CgBlend layer;
				
				QString source( QString::fromUtf8( it->attribute( "src" ).value() ) );
				layer.x = it->attribute( "x" ).as_int( 0 );
				layer.y = it->attribute( "y" ).as_int( 0 );
				
				auto src_name = it->attribute( "src" ).value();
				layer.primitive_id = image.findPrimitive( src_name );
					
				//composite-op
				layer.mode = blendTypeFromString( it->attribute( "composite-op" ).value() );
				
				frame.layers.push_back(layer);
			}
			else
				std::cout << "Unrecognized element: " << it->name() << '\n';

		image.addFrame(frame);
		
	}while( (stack = stack.next_sibling("stack")) );
	
	if( !image.isValid() )
		throw std::runtime_error( "CgImage contained errors or is otherwise invalid" );
	
	return image;
}