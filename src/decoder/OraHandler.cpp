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

#include <cstring>
#include <iterator>

#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QBuffer>
#include <QVariant>

#include <archive_entry.h>

using namespace pugi;

static QByteArray read_data( archive* a ){
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

static RgbaImage read_image( archive* a, const char* format=nullptr ){
	return FormatWebP::readRgbaImage( read_data( a ) );
}

static QString next_file( archive* a ){
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

bool OraHandler::read_and_validate( archive *a ){
	//Read and check mime
	QString mime = next_file( a );
	if( mime.isNull() ){
		qWarning( "Ora image is not an usable zip archive" );
		return false;
	}
		
	if( mime != "mimetype" ){
		qWarning( "Ora image must start with the mimetype" );
		return false;
	}
	//TODO: check if it is STORED?
	
	QByteArray mime_data = read_data( a );
	if( mime_data != "image/openraster" ){
		qWarning( "Mimetype does not match" );
		return false;
	}
	
	//Read all files
	QString name;
	while( !(name = next_file( a )).isNull() ){
		if( name.startsWith( "data/" ) ){
			QString suffix = QFileInfo(name).suffix();
			auto img = read_image( a, suffix.toLocal8Bit().constData() );
			if( img.width() != 0 )
				images.insert( {name, std::move(img) } );
			else
				qWarning( "Could not read data file: %s", name.toLocal8Bit().constData() );
		}
		else if( name == "stack.xml" ){
			QByteArray data = read_data( a );
			if( !doc.load_buffer( data.data(), data.size() ) )
				return false;
		}
		else
			qWarning( "Unknown file: %s", name.toLocal8Bit().constData() );
	}
	//TODO: check for stack.xml
	
	return true;
}

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

bool OraHandler::load(QIODevice& device){
	loaded = true;
	bool success = true;
	archive* a = archive_read_new();
	archive_read_support_format_zip(a);
	
	ReadingData data( device );
	if( archive_read_open( a, &data, nullptr, stream_read, stream_close ) ){
		qDebug( "couldn't open: %s", archive_error_string(a) );
		success = false;
	}
	else
		success = read_and_validate( a );
	
	archive_read_close( a );
	archive_read_free( a );
	
	return success;
}

static void alpha_replace( RgbaView output, ConstRgbaView image, int dx, int dy ){
	Blending::BlendImages<Rgba>( output.crop( dx, dy, image.width(), image.height() ), image, Blending::alphaReplace );
}

static void src_over( RgbaView output, ConstRgbaView image, int dx, int dy ){
	Blending::BlendImages<Rgba>( output.crop( dx, dy, image.width(), image.height() ), image, Blending::srcOverRgba );
}

void OraHandler::render_stack( xml_node node, RgbaImage &output, int offset_x, int offset_y ) const{
	for( xml_node_iterator it = --node.end(); it != --node.begin(); it-- ){
		std::string name( (*it).name() );
		if( name == "stack" ){
			int x = (*it).attribute( "x" ).as_int( 0 );
			int y = (*it).attribute( "y" ).as_int( 0 );
			
			render_stack( *it, output, offset_x+x, offset_y+y );
		}
		else if( name == "layer" ){
			QString source( QString::fromUtf8( (*it).attribute( "src" ).value() ) );
			int x = (*it).attribute( "x" ).as_int( 0 ) + offset_x;
			int y = (*it).attribute( "y" ).as_int( 0 ) + offset_y;
			
			std::string visibility = (*it).attribute( "visibility" ).value();
			if( visibility == "" || visibility == "visible" ){
				auto img_it = images.find( source );
				if( img_it == images.end() ){
					qWarning( "Layer source not found: %s", source.toLocal8Bit().constData() );
					return;
				}
				auto& image = img_it->second;
					
				//composite-op
				std::string composite = (*it).attribute( "composite-op" ).value();
				if( composite == "cgcompress:alpha-replace" )
					alpha_replace( output, image, x, y );
				else if( composite == "svg:src-over" || composite == "" ){
					src_over( output, image, x, y );
				}
				else{
					qWarning( "Unsupported composite-op: %s", composite.c_str() );
					return;
				}
			}
		}
		else{
			qWarning( "Unrecognized element: %s", name.c_str() );
		}
	}
}

RgbaImage OraHandler::read(){
	frame++;
	
	if( !loaded )
		return {};
	
	xml_node img = doc.child( "image" );
	int width = img.attribute( "w" ).as_int( -1 );
	int height = img.attribute( "h" ).as_int( -1 );
	if( width <= 0 || height <= 0 ){
		qWarning( "Image dimensions are invalid" );
		return {};
	}
	
	xml_node stack = img.child( "stack" );
	for( int i=0; i<frame; i++ )
		stack = stack.next_sibling( "stack" );
	if( !stack )
		return {};
	
	RgbaImage output( width, height );
	render_stack( stack, output );
	return output;
}

int OraHandler::imageCount() const{
	//Counts the amount of "stack" elements in "image"
	auto frames = doc.child( "image" ).children( "stack" );
	return std::distance( frames.begin(), frames.end() );
}
/*
int OraHandler::nextImageDelay() const{
	int wanted = frame + 1 >= imageCount() ? 0 : frame + 1;
	
	//Iterate to the node we want
	auto node = doc.child( "image" ).children( "stack" ).begin();
	while( wanted > 0 ){
		node++;
		wanted--;
	}
	
	return node->attribute( "delay" ).as_int( 100 );
}*/

