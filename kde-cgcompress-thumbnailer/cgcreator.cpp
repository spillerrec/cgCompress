#include "cgcreator.hpp"

#include <archive.h>
#include <archive_entry.h>

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>

#include <QLoggingCategory>
Q_LOGGING_CATEGORY(LOG, "cgcompress.thumbnailer.spillerrec")

extern "C"{
	Q_DECL_EXPORT ThumbCreator *new_creator(){ return new CgCompressCreator; }
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

static QString next_file( archive* a ){
	archive_entry *entry;
	switch( archive_read_next_header( a, &entry ) ){
		case ARCHIVE_EOF: return QString();
		case ARCHIVE_OK:
			return QString::fromWCharArray( archive_entry_pathname_w( entry ) );
			
		default:
			qCWarning(LOG) << "Can't read the next zip header:" << archive_error_string(a);
			return QString();
	}
}

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
				qCWarning(LOG) << "Error while reading zip data:" << archive_error_string(a);
				return raw;
		}
	}
}

static QImage read_image( archive* a, const char* format=nullptr ){
	QByteArray raw = read_data( a );
	QBuffer buf( &raw );
	QImageReader reader( (QIODevice*)&buf, format );
	return reader.canRead() ? reader.read() : QImage();
}



bool CgCompressCreator::create( const QString& path, int, int, QImage& img ){
	qCDebug(LOG) << "Trying to read image" << path;
	QFile file(path);
	if( !file.open( QIODevice::ReadOnly ) )
		return false;
	
	archive* a = archive_read_new();
	archive_read_support_format_zip(a);
	
	ReadingData data( file );
	if( archive_read_open( a, &data, nullptr, stream_read, stream_close ) )
		qCWarning(LOG) << "couldn't open:" << archive_error_string(a);
	else{
		QString name;
		while( !(name = next_file( a )).isNull() ){
			if( name.startsWith( "Thumbnails/thumbnail." ) ){
				qCDebug(LOG) << "Found thumbnail!";
				QString suffix = QFileInfo(name).suffix();
				img = read_image( a, suffix.toLocal8Bit().constData() );
				break;
			}
		}
	}
	
	archive_read_close( a );
	archive_read_free( a );
	
	if( img.isNull() )
		qCWarning(LOG) << "No thumbnail found!";
	return !img.isNull();
}
