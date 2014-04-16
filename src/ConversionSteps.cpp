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

#include "ConversionSteps.hpp"
#include "ListFunc.hpp"

QList<Conversion> ConversionSteps::optimizedConverters( QList<Conversion> in ) const{
	QList<Converter const*> converters;
	for( auto conv : in )
		if( !converters.contains( conv.getConverter() ) )
			converters << conv.getConverter();
	
	
}

QList<Conversion> ConversionSteps::usableConvertions() const{
	QList<Conversion> convs;
	for( const auto& converter : converters ){
		for( auto step : converter.getSteps() ){
			//Can't use this if we cannot get to its 'from' image
			if( !has.contains( step.from ) )
				continue;
			
			//Not useful if we already can get to its 'to' image
			if( has.contains( step.to ) )
				continue;
			
			//TODO: correctly calculate file size in case of several steps
			//We can get negative file sizes, in case it makes another converter redundant
			Conversion conv( converter, step.from, step.to, converter.get_size() );
			convs << conv;
		}
	}
	
	return convs;
}

void ConversionSteps::addBaseImage( int base ){
	for( const auto& converter : converters ){
		for( auto step : converter.getSteps() ){
			if( step.from == step.to && step.from == base ){
				addConvertion( Conversion( converter, base, base, converter.get_size() ) );
				return;
			}
		}
	}
}

void ConversionSteps::findBestConverters(){
	while( amount > has.size() ){
		auto convs = usableConvertions();
		if( convs.size() == 0 )
			qDebug( "Shit, no usableConvertions" );
		//TODO: exception if no convs?
		addConvertion( *minimum( convs ) );
	}
}


int ConversionSteps::fileSize() const{
	int total = 0;
	for( auto conv : conversions )
		total += conv.size;
	return total;
}

#include <QDebug>
Conversion ConversionSteps::convertionTo( int image ) const{
	for( auto conv : conversions )
		if( conv.to == image )
			return conv;
	//TODO: throw exception
	qDebug() << "convertionTo: shit";
	return conversions[0];
}

QList<Conversion> ConversionSteps::getConversionsTo( int image ) const{
	QList<Conversion> convs;
	
	Conversion current = convertionTo( image );
	QList<Conversion> pre;
	while( true ){
		pre << current;
		if( current.from == current.to )
			//We reached a full image, stop
			break;
		else{
			//Find another converter which leads to this one
			for( auto conv : conversions )
				if( conv.to == current.from ){
					//Make sure we don't reuse converters, causing infinite loop
					//TODO: really needed?
					bool can_use = true;
					for( auto p : pre )
						if( p == conv )
							can_use = false;
					if( can_use ){
						current = conv;
						break;
					}
				}
		}
	}
	//Add in reverse
	while( pre.size() > 0 )
		convs << pre.takeLast();
			
	return convs;
}


void ConversionSteps::addConvertion( Conversion c ){
	if( c.getConverter()->getSteps().size() == 1 ){
		conversions << c;
		has << c.to;
	}
	else
		qDebug( "TODO: optimize converters" );
	//TODO: when to remove unneeded converters?
}

