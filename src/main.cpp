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
#include <QDebug>

#include "Format.hpp"
#include "MultiImage.hpp"
#include "FileUtils.hpp"
#include "ImageOptim.hpp"

#include <iostream>
using namespace std;

/** Print version number to stdout */
static void print_version(){
	cout << "cgCompress - version 1.0.0 (beta)" << endl;
}

/** Print usage info to stdout */
static void print_help(){
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
	cout << "\t" << "--combined     Combine several cgCompress files into one" << endl;
	cout << "\t" << "--noalpha      Remove alpha channel from input images" << endl;
	cout << "\t" << "--discard-transparent  Remove pixel values from transparent pixels" << endl;
	cout << "\t" << "--evaluate     Write a CSV file which evaluates filesize compared to other formats" << endl;
}

/** Retrieves XXX from --name=XXX
 *  
 *  \param [in] options Arguments supplied to program
 *  \param [in] name of the parameter, without "--" and "="
 *  \param [in] default_value Value if argument was not found
 *  \return The value of XXX, or QString() if not found
 */
static QString get_option_value( QStringList options, QString name, QString default_value={} ){
	name = "--" + name + "=";
	for( auto opt : options )
		if( opt.startsWith( name ) )
			return opt.right( opt.size() - name.size() );
	return default_value;
}

static QString parse_format( QString input ){
	auto supported = QImageReader::supportedImageFormats();
	
	input = input.toLower();
	if( !input.isEmpty() ){
		if( supported.contains( input.toLatin1() ) )
			return input;
		else
			qWarning( "Format '%s' not supported, using default", input.toLocal8Bit().constData() );
	}
	
	//Use webp as default if available, otherwise use png
	return supported.contains( "webp" ) ? "webp" : "png";
}

static int parse_int( QString input, int default_value=0 ){
	if( input.isEmpty() )
		return default_value;
	
	bool ok = false;
	int converted = input.toInt( &ok );
	if( ok )
		return converted;
	
	qDebug( "Integer '%s' not valid, using default of '%d'", input.toLocal8Bit().constData(), default_value );
	return default_value;
}

static int optimizeImage( MultiImage& img, QString output_path ){
	img.optimize( output_path );
	if( !img.validate( output_path + ".cgcompress" ) ){
		//Issue with file, don't convert
		cout << "Resulting file did not pass validity check!\n";
		QFile::remove( output_path );
		std::getchar();
		return -1;
	}
	return 0;
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
	
	//Get user-defined format
	Format format( parse_format( get_option_value( options, "format" ) ) );
	
	//Get quality
	format.set_precision( parse_int( get_option_value( options, "quality" ), 1 ) );
	
	//An optional string to append to the end of newly created files
	//TODO: might not be used everywhere
	auto name_extension = get_option_value( options, "name-extension" );
	
	auto apply_transformations = [&]( RgbaView img ){
		if( options.contains( "--discard-transparent" ) )
			removeTransparent(img);
		
		if( options.contains( "--noalpha" ) )
			removeAlpha(img);
	};
	
	if(      options.contains( "--help"    ) ) print_help();
	else if( options.contains( "--version" ) ) print_version();
	else if( options.contains( "--pack"    ) ){
		if( name_extension.isNull() )
			name_extension = ".packed";
		for( auto file : files )
			pack_directory( file, name_extension );
	}
	else if( options.contains( "--evaluate" ) ){
		evaluate_cgcompress( expandFolders( files ) );
	}
	else if( options.contains( "--diff-test" ) ){
		auto file1 = Format::read( files[0] );
		auto file2 = Format::read( files[1] );
		ImageDiff( file2, file1, 3 );
	}
	else if( options.contains( "--diff-test-apply" ) ){
		auto file1 = Format::read( files[0] );
		auto file2 = Format::read( "ImageDiff-diff.webp" );
		auto file3 = Format::read( "ImageDiff-overlay.webp" );
		ImageDiffCombine( file1, file2, file3 );
		format.save( file1, "ImageDiff-result" );
	}
	else if( options.contains( "--extract" ) ){
		for( auto file : files )
			extract_cgcompress( file, format );
	}
	else if( options.contains( "--recompress" ) ){
		if( name_extension.isNull() )
			name_extension = ".recompressed";
		files = expandFolders( files );
		for( auto file : files ){
			auto images = extract_files( file );
			for( auto& image : images)
				apply_transformations( image.second );
			QString name( QFileInfo(file).completeBaseName() + name_extension );
			
			MultiImage multi_img( format );
			for( auto& image : images )
				multi_img.append( Image( image.second ) );
			
			optimizeImage( multi_img, name );
		}
	}
	else if( options.contains( "--combined" ) ){
		MultiImage multi_img( format );
		for( auto file : files )
			for( auto& image : extract_files( file ) ){
				apply_transformations( image.second );
				multi_img.append( Image( image.second ) );
			}
		
		optimizeImage( multi_img, QFileInfo(files[0]).completeBaseName() + name_extension );
	}
	else{
		files = expandFolders( files );
		
		if( files.size() < 2 ){
			if( files.size() == 0 )
				print_help();
			else
				cout << "Needs at least two files in order to compress";
			return -1;
		}
		
		for( int start=0; start<files.size(); ){
			auto name = QFileInfo(files[start]).completeBaseName();
			qDebug() << "Compressing " << name;
			MultiImage multi_img( format );
			
			std::vector<RgbaImage> images;
			ConstRgbaView last;
			for( int j=start; j<files.size(); j++ ){
				images.push_back( fromQImage( QImage( files[j] ) ) );
				//TODO: apply_transformations
				
				if( options.contains( "--auto" ) && last.valid() && !isSimilar( images.back(), last ) )
					break;
				
				last = images.back();
				multi_img.append( Image( last ) );
			}
			
			optimizeImage( multi_img, name + name_extension );
			start += multi_img.count();
		}
	}
	
	return 0;
}