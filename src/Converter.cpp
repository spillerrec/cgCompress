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

#include "Converter.hpp"

#include <algorithm>
#include <stdexcept>

Image Converter::get_primitive() const{
	//No diff if no conversion can be made
	if( from == to )
		return (*base_images)[from];
	
	return (*base_images)[from].difference( (*base_images)[to] );
}

std::vector<int> Converter::path( const QList<Converter>& converters, int from, int to ){
	std::vector<int> conv_path;
	conv_path.push_back( from );
	
	for( int current = from; current != to; ){
		//Find matching converter
		auto is_match = [&](auto conv){ return conv.get_to() == current; };
		auto match = std::find_if( converters.begin(), converters.end(), is_match );
		if( match == converters.end() )
			throw std::runtime_error( "Converter::path() could not find a complete path" );
		
		current = match->get_from();
		conv_path.push_back( current );
	}
	
	//Reverse list, from https://stackoverflow.com/a/20652805/2248153
	for( int k=0, s=conv_path.size(), max=s/2; k<max; k++ )
		std::swap( conv_path[k], conv_path[s-1-k] );
	
	return conv_path;
}
