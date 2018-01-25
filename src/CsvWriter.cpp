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

#include "CsvWriter.hpp"

static auto SEPERATOR = ",";
static auto STRING_QUOTE = "'";


CsvWriter::CsvWriter( QString path, QStringList headers ) : file( path ){
	auto exists = file.exists(); //Will this info be changed by the open command?
	
	if( !file.open( QIODevice::Append | QIODevice::Text ) )
		throw std::runtime_error( "CSV file couldn't be opened for writing!" ); //TODO: Warning only?
	
	//Write headers if it is a new file, or continue writing new lines in an existing file
	//TODO: Use filesize as a better indicator?
	if( !exists ){
		for( auto header : headers )
			write( header );
		stop();
	}
}


CsvWriter& CsvWriter::writeRaw( QString value ){
	file.write( value.toUtf8().constData() ); //NOTE: Here is locale defined!
	file.write( SEPERATOR );
	return *this;
}

CsvWriter& CsvWriter::write( QString item ){
	//TODO: only do if spaces are in string option?
	writeRaw( STRING_QUOTE + item + STRING_QUOTE );
	//TODO: Performance compared to writing a string first?
	return *this;
}

CsvWriter& CsvWriter::write( int value ){
	writeRaw( QString::number( value ) );
	return *this;
}

CsvWriter& CsvWriter::write( double value ){
	writeRaw( QString::number( value ) );
	return *this;
}

void CsvWriter::stop(){
	file.write( "\n" );
	file.flush();
}


