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

#ifndef CSV_WRITER_HPP
#define CSV_WRITER_HPP

#include <QList>
#include <QFile>
#include <QString>
#include <QStringList>

class CsvWriter {
	private:
		QFile file;
		
	public:
		CsvWriter( QString path, QStringList headers );
		
		CsvWriter& writeRaw( QString value );
		CsvWriter& write( QString item );
		CsvWriter& write( int value );
		CsvWriter& write( double value );
		void stop();
		
		//TODO: Variatic template
};

#endif

