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

#include <QCoreApplication>
#include <QStringList>
#include <QFileInfo>
#include <QImageReader>

#include "Format.hpp"
#include "MultiImage.hpp"
#include "FileUtils.hpp"
#include "ListFunc.hpp"

#include <iostream>
using namespace std;

/** Print version number to stdout */
void print_version(){
	cout << "cgCompress - version 0.0.1 (pre-alpha)" << endl;
}

/** Print usage info to stdout */
void print_help(){
	cout << "Usage:" << endl;
	cout << "cgCompress [options] [files]" << endl;
	cout << endl;
	cout << "Options:" << endl;
	cout << "\t" << "--extract      Uncompress cgCompress files" << endl;
	cout << "\t" << "--quality=X    0 provides best compression, higher values are faster but larger filesize" << endl;
	cout << "\t" << "--format=XXX   Use format XXX for compressing/extracting" << endl;
	cout << "\t" << "--help         Show this help" << endl;
	cout << "\t" << "--pack         Re-zip an unzipped cgCompress file" << endl;
	cout << "\t" << "--recompress   Extract and recompress a cgCompress file" << endl;
	cout << "\t" << "--version      Show program version" << endl;
}

/** Retrieves XXX from --name=XXX
 *  
 *  \param [in] options Arguments supplied to program
 *  \param [in] name of the parameter, without "--" and "="
 *  \return The value of XXX, or QString() if not found
 */
QString get_option_value( QStringList options, QString name ){
	name = "--" + name + "=";
	for( auto opt : options )
		if( opt.startsWith( name ) )
			return opt.right( opt.size() - name.size() );
	return QString();
}

/** Retrieve format settings from the input arguments.
 *  --format=XXX changes format
 *  --quality=X changes quality of estimation
 *  
 *  \param [in] options Arguments supplied to program
 *  \return Format XXX, default if not supplied
 */
Format get_format( QStringList options ){
	auto supported = QImageReader::supportedImageFormats();
	
	//Get quality
	int level = get_option_value( options, "quality" ).toInt();
	
	//Get user-defined format
	QString format = get_option_value( options, "format" ).toLower();
	if( !format.isEmpty() ){
		if( supported.contains( format.toLatin1() ) ){
			Format f( format );
			f.set_precision( level );
			return f;
		}
		else
			qWarning( "Format '%s' not supported, using default", format.toLocal8Bit().constData() );
	}
	
	//Use webp as default if available, otherwise use png
	Format f( supported.contains( "webp" ) ?	"webp" : "png" );
	f.set_precision( level );
	return f;
}

int main( int argc, char* argv[] ){
	QCoreApplication app( argc, argv );
	
	QStringList args = app.arguments();
	args.removeFirst();
	
	QStringList options, files;
	for( auto arg : args )
		if( arg.startsWith( "--" ) )
			options << arg;
		else
			files << arg;
	
	Format format = get_format( options );
	
	if( options.contains( "--help" ) ){
		print_help();
		return 0;
	}
	else if( options.contains( "--version" ) ){
		print_version();
		return 0;
	}
	else if( options.contains( "--pack" ) ){
		for( auto file : files )
			pack_directory( file );
		return 0;
	}
	else if( options.contains( "--extract" ) ){
		for( auto file : files )
			extract_cgcompress( file, format );
		return 0;
	}
	else if( options.contains( "--recompress" ) ){
		for( auto file : files ){
			auto images = extract_files( file );
			QList<Image> imgs;
			for( auto image : images )
				imgs << Image( {0,0}, image.second );
			
			MultiImage::optimized( format, imgs ).save( QFileInfo(file).baseName() + ".recompressed" );
		}
		
		return 0;
	}
	else{
		if( files.size() < 2 ){
			if( files.size() == 0 )
				print_help();
			else
				cout << "Needs at least two files in order to compress";
			return -1;
		}
		
		MultiImage::optimized( format
			,	map2<Image>( files, [](QString info){ return Image(info); } )
			).save( QFileInfo(files[0]).baseName() );
		
		return 0;
	}
}