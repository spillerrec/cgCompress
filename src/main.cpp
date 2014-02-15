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
	cout << "\t" << "--extract      Uncompress cgcompress files" << endl;
	cout << "\t" << "--format=XXX   Use format XXX for compressing/extracting" << endl;
	cout << "\t" << "--help         Show this help" << endl;
	cout << "\t" << "--pack         Re-zip an unzipped cgcompress file" << endl;
	cout << "\t" << "--version      Show program version" << endl;
}

/** Retrieve format from input arguments in the form "--format=XXX".
 *  
 *  \param [in] options Arguments supplied to program
 *  \return Format XXX, default if not supplied
 */
Format get_format( QStringList options ){
	auto supported = QImageReader::supportedImageFormats();
	
	//Check for user-defined format
	for( auto opt : options )
		if( opt.startsWith( "--format=" ) ){
			QString format = opt.right( opt.size() - 9 ).toLower();
			if( supported.contains( format.toLocal8Bit() ) )
				return format;
			else{
				qWarning( "Format '%s' not supported, using default", format.toLocal8Bit().constData() );
				break;
			}
		}
	
	//Use webp as default if available, otherwise use png
	return supported.contains( "webp" )
		?	"webp" : "png";
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
		cout << "Unimplemented";
		return 0;
	}
	else if( options.contains( "--extract" ) ){
		cout << "Unimplemented";
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
		
		MultiImage multi_img;
		for( auto file : files )
			multi_img.append( Image( file ) );
			
		multi_img.set_format( format );
		
		auto frames = multi_img.optimize2( QFileInfo(files[0]).baseName() );
		
		//TODO: save frames
		
		return 0;
	}
}