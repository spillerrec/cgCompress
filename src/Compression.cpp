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

#include "Compression.hpp"

#include <lz4.h>
#include <lz4hc.h>

#include <vector>

namespace FileSize{

int lz4compress_size( const unsigned char* data, unsigned size ){
	auto max_size = LZ4_compressBound( size );
	std::vector<unsigned char> buffer( max_size );
	
	return LZ4_compress_HC(
			(const char*)data, (char*)buffer.data()
		,	size, buffer.size()
		,	LZ4HC_DEFAULT_CLEVEL //TODO: Higher?
		);
}

}
