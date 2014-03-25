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

#ifndef LIST_FUNC_HPP
#define LIST_FUNC_HPP

#include <algorithm>

template<class T1, class UnaryOperator>
T1 filter( T1 list, UnaryOperator op ){
	T1 output( list );
	auto last = std::remove_copy_if( list.begin(), list.end(), output.begin(), op );
	output.erase( last, output.end() );
	return output;
}

template<class T1, class UnaryOperator>
T1 filter_inpl( T1& list, UnaryOperator op ){
	auto last = std::remove_if( list.begin(), list.end(), op );
	list.erase( last, list.end() );
	return list;
}

/*/TODO: how to make output general?
template<class T1, class BinaryOperation>
T1 map( const T1& list, BinaryOperation bop ){
	T1 output( list );
	std::transform( list.begin(), list.end(), output.begin(), bop );
	return output;
}
//*/
//QList implementation
template<typename T2, typename T1, class BinaryOperation>
QList<T2> map( const QList<T1>& list, BinaryOperation bop ){
	QList<T2> output;
	output.reserve( list.size() );
	for( auto e : list )
		output.append( bop( e ) );
	return output;
}

template<typename T2, typename T1, class BinaryOperation>
QList<T2> map2( const QList<T1>& list, BinaryOperation bop ){
	QList<T2> output;
	output.reserve( list.size() );
	for( auto e : list )
		output.append( bop( e ) );
	return output;
}

template<class T1, class Function>
void for_each( T1 list, Function fn ){
	std::for_each( list.begin(), list.end(), fn );
}

//TODO: we want these to use const T&
template<class T>
auto maximum( T& list ) -> decltype( list.begin() ){
	return std::max_element( list.begin(), list.end() );
}
template<class T, class Compare>
auto maximum( T& list, Compare comp ) -> decltype( list.begin() ){
	return std::max_element( list.begin(), list.end(), comp );
}

template<class T>
auto minimum( T& list ) -> decltype( list.begin() ){
	return std::min_element( list.begin(), list.end() );
}
template<class T, class Compare>
auto minimum( T& list, Compare comp ) -> decltype( list.begin() ){
	return std::min_element( list.begin(), list.end(), comp );
}

template<class T1, class UnaryOperator>
void sort( T1& list, UnaryOperator op ){
	std::sort( list.begin(), list.end(), op );
}

#endif

