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

#ifndef FORMAT_HPP
#define FORMAT_HPP

#include <QImage>
#include <QString>
#include <QByteArray>
#include <QBuffer>

/** Handles format and quality settings for image formats. */
class Format {
	private:
		QByteArray format{ "png" }; ///file extension compatible with Qt format
		int quality{ 100 }; ///Compression quality when saving
		
	public:
		Format() { }
		Format( QByteArray format ) : format(format) { }
		Format( QByteArray format, int quality ) : format(format), quality(quality) { }
		Format( const char* format ) : format(format) { }
		Format( QString format ) : format( format.toLocal8Bit() ) { }
		
		/** Compression quality
		 *  \return quality
		 */
		int get_quality() const{ return (format=="png") ? -1 : quality; }
		
		/** Lossy version of this format
		 *  \return Format with lossy compression
		 */
		Format get_lossy(){
			if( format=="webp" )
				return Format( format, 95 );
			else
				return Format( "jpg", 95 );
		}
		
		/** Create filename with a compatible extension
		 *  \param [in] name Name of the file
		 *  \return Name with extension
		 */
		QString filename( QString name ) const{
			return name + "." + format;
		}
		
		/** Save image to the file system compressed
		 *  
		 *  \param [in] img Image to save
		 *  \param [in] path File system path, without extension
		 *  \return true on sucess
		 */
		bool save( QImage img, QString path ) const{
			return img.save( filename(path), ext(), get_quality() );
		}
		
		/** Compress image to a memory buffer
		 *  
		 *  \param [in] img Image to save
		 *  \return buffer containing the compressed image
		 */
		QByteArray to_byte_array( QImage img ) const{
			QByteArray data;
			QBuffer buffer( &data );
			buffer.open( QIODevice::WriteOnly );
			img.save( &buffer, ext(), get_quality() );
			return data;
		}

		
		const char* ext() const{ return format.constData(); }
};

#endif

