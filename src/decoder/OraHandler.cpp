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
#include "../Image.hpp"
#include "../Frame.hpp"
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
			auto buffer = a.read_data();
			std::cout << "File size: " << buffer.size() << '\n';
			pri.img =  FormatWebP::readRgbaImage( buffer );
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


#include "../Format.hpp"
#include "../minizip/zip.h"
#include <boost/range/adaptor/reversed.hpp>
#include <QString>
#include <QDate>

static void setTime( tm_zip &tm, QDate date, QTime time ){
	tm.tm_sec  = time.second();
	tm.tm_min  = time.minute();
	tm.tm_hour = time.hour();
	tm.tm_mday = date.day();
	tm.tm_mon  = date.month();
	tm.tm_year = date.year();
}

static bool addByteArray( zipFile &zf, QString name, QByteArray arr, int compression=0 ){
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

static bool addStringFile( zipFile &zf, QString name, QString contents, bool compress=false ){
	return addByteArray( zf, name, contents.toUtf8(), compress ? 9 : 0 );
}

static std::string createStack( const CgImage& image ){
	QString stack( "<?xml version='1.0' encoding='UTF-8'?>\n" );
	stack += QString( "<image w=\"%1\" h=\"%2\">" ).arg( image.getWidth() ).arg( image.getHeight() );
	
	for( int i=0; i<image.frameCount(); i++ ){
		stack += "<stack>";
		
		auto& frame = image.getFrame( i );
		for( auto& layer : boost::adaptors::reverse(frame.layers) ){
			QString composition = "composite-op=\"cgcompress:alpha-replace\"";
			switch( layer.mode ){
				case CgBlendType::SRC_OVER:      composition=""; break;
				case CgBlendType::ALPHA_REPLACE: composition="composite-op=\"cgcompress:alpha-replace\""; break;
				case CgBlendType::FAST_OVER:     composition="composite-op=\"cgcompress:fast\""; break;
				case CgBlendType::DIFF_ADD:      composition="composite-op=\"cgcompress:diff-add\""; break;
				case CgBlendType::COPY_FRAME:    composition=""; break;
				default: std::terminate();
			}
			
			if( layer.mode != CgBlendType::COPY_FRAME ){
				stack += QString( "<layer %1 name=\"%2\" src=\"%2\" x=\"%3\" y=\"%4\" />" )
					.arg( composition )
					.arg( image.getPrimitve(layer.primitive_id).filename.c_str() )
					.arg( layer.x )
					.arg( layer.y ) //TODO: width/height?
					;
			}
			else{
				//TODO:
				qDebug( "Copy frame not yet implemented" );
				std::terminate();
			}
		}
		
		stack += "</stack>";
	}
	
	stack += "</image>";
	
	return stack.toUtf8().constData();
}


bool saveCgImage( const CgImage& image, QString path ){
	if( !image.isValid() )
		return false;
	
	qDebug("opening\n");
	//TODO: make wrapper class for zipFile
	zipFile zf = zipOpen64( path.toLocal8Bit().constData(), 0 );
	
	//Save mimetype without compression
	addStringFile( zf, "mimetype", "image/x-cgcompress" );
	
	//Save stack with compression
	addStringFile( zf, "stack.xml", createStack(image).c_str(), true );
	
	/* TODO: Thumbnail
	auto first_frame = frames.front().reconstruct();
	auto thumb = fromQImage( toQImage(first_frame).scaled( 256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
	Format lossy = format.get_lossy();
	files.append( { lossy.filename("Thumbnails/thumbnail"), lossy.to_byte_array( thumb ) } );
	*/
	
	//Save all data files
	for( int i=0; i<image.primitiveCount(); i++ ){
		auto& pri = image.getPrimitve(i);
		addByteArray( zf, pri.filename.c_str(), Format( "webp" ).to_byte_array( pri.img ) ); //TODO: Format
		//TODO: compress if there are significant savings. Perhaps user defined threshold?
	}
	
	qDebug("closing\n");
	zipClose( zf, NULL );
	return true;
}

bool saveCgImage( class QString path
	,	int width, int height, Format format
	,	const std::vector<Image>& primitives
	,	const std::vector<Frame>& frames
){
	CgImage cg;
	cg.setDimensions( width, height );
	
	//Find all used layers, and return a sorted list of the used ids
	std::vector<int> used_layers;
	for( auto& frame : frames )
		for( auto layer : frame.layers )
			used_layers.push_back( layer );
	std::sort( used_layers.begin(), used_layers.end() );
	used_layers.erase( std::unique( used_layers.begin(), used_layers.end() ), used_layers.end() );
	
	//reverse lookup
	std::vector<int> layer_to_id( primitives.size(), -1 );
	int index=0;
	for( auto used : used_layers )
		layer_to_id.at(used) = index++;
	
	//convert...
	std::vector<CgBlend> blends;
	for( size_t i=0; i<primitives.size(); i++ ){
		auto& img = primitives[i];
		CgBlend blend;
		blend.primitive_id = layer_to_id[i];
		blend.x      = img.get_pos().x();
		blend.y      = img.get_pos().y();
		blend.width  = img.view().width();
		blend.height = img.view().height();
		blend.mode   = CgBlendType::ALPHA_REPLACE;
		blends.push_back( blend );
	}
	
	for( auto id : used_layers )
		cg.addPrimitive( primitives[id].to_primitive() );
	for( auto& frame : frames ){
		CgFrame cgframe;
		
		for( auto layer : frame.layers )
			cgframe.layers.push_back( blends.at(layer) );
		
		cg.addFrame( cgframe );
	}
	
	cg.autosetPrimitiveNames( format );
	return saveCgImage( cg, path );
}
