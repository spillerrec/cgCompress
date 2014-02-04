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

#include "Image.hpp"

#include <cmath>

#include <QPainter>
#include <QBuffer>

using namespace std;

int Image::compressed_size( const char* format ) const{
	return to_byte_array( format ).size();
}

QByteArray Image::to_byte_array( const char* format ) const{
	QByteArray data;
	QBuffer buffer( &data );
	buffer.open( QIODevice::WriteOnly );
	img.save( &buffer, format );
	return data;
}

bool content_in_vertical_line( QImage img, int x ){
	for( int iy=0; iy<img.height(); iy++ )
		if( qAlpha( img.pixel(x,iy) ) != 0 )
			return true;
	return false;
}

QList<Image> Image::segment() const{
	QList<Image> images;
	
	//Check vertical lines
	bool content = content_in_vertical_line( img, 0 );
	int first_line = 0;
	for( int ix=1; ix<img.width(); ix++ ){
		bool cur_content = content_in_vertical_line( img, ix );
		if( content == cur_content )
			continue;
		
		if( cur_content == true )
			first_line = ix;
		else
			images.append( sub_image( first_line,0, ix-first_line, img.height() ) );
		
		content = cur_content;
	}
	if( content )
		images.append( sub_image( first_line,0, img.width()-first_line, img.height() ) );
	//TODO: grab the last image if content touches the right edge
	
	//TODO: check horizontal lines
	//TODO: make recursive?
	
	//TODO: make sure this doesn't bloat the file size with many small images.
	//   A cost function which models the overhead could be used.
	
	return images;
}

Image Image::combine( Image on_top ) const{
	QPoint tl{ min( pos.x(), on_top.pos.x() ), min( pos.y(), on_top.pos.y() ) };
	int width = max( pos.x()+img.width(), on_top.pos.x()+on_top.img.width() ) - tl.x();
	int height = max( pos.y()+img.height(), on_top.pos.y()+on_top.img.height() ) - tl.y();
	
	QImage output( width, height, QImage::Format_ARGB32 );
	output.fill( 0 );
	QPainter painter( &output );
	painter.drawImage( pos-tl, img );
	painter.drawImage( on_top.pos-tl, on_top.img );
	
	return Image( tl, output );
}

Image Image::difference( Image input ) const{
	//TODO: images must be the same size and at same point
	
	QImage output( img.convertToFormat(QImage::Format_ARGB32) );
	
	for( int iy=0; iy<output.height(); iy++ ){
		QRgb* out = (QRgb*)output.scanLine( iy );
		const QRgb* in = (const QRgb*)input.img.constScanLine( iy );
		for( int ix=0; ix<output.width(); ix++ ){
			if( qAlpha( in[ix] ) == 255 && qAlpha( out[ix] ) == 255 ){
				if( in[ix] == out[ix] )
					out[ix] = qRgba( qRed(in[ix]),qGreen(in[ix]),qBlue(in[ix]),0 );
				else
					out[ix] = in[ix];
			}
			else
				out[ix] = in[ix];
		}
	}
	
	return Image( {0,0}, output );
}

Image Image::remove_transparent() const{
	QImage output( img.convertToFormat(QImage::Format_ARGB32) );
	
	for( int iy=0; iy<output.height(); iy++ ){
		QRgb* out = (QRgb*)output.scanLine( iy );
		for( int ix=0; ix<output.width(); ix++ )
			if( qAlpha( out[ix] ) == 0 )
				out[ix] = qRgba( 0,0,0,0 );
	}
			
	return Image( pos, output );
}

Image Image::auto_crop() const{
	int right=0, left=0;
	int top=0, bottom=0;
	
	//Decrease top
	for( ; top<img.height(); top++ ){
		const QRgb* row = (const QRgb*)img.constScanLine( top );
		for( int ix=0; ix<img.width(); ix++ )
			if( qAlpha( row[ix] ) != 0 )
				goto TOP_BREAK;
	}
TOP_BREAK:
	
	//Decrease bottom
	for( ; bottom<img.height(); bottom++ ){
		const QRgb* row = (const QRgb*)img.constScanLine( img.height()-1-bottom );
		for( int ix=0; ix<img.width(); ix++ )
			if( qAlpha( row[ix] ) != 0 )
				goto BOTTOM_BREAK;
	}
BOTTOM_BREAK:
	
	//Decrease left
	for( ; left<img.width(); left++ )
		for( int iy=0; iy<img.height(); iy++ )
			if( qAlpha( img.pixel(left,iy) ) != 0 )
				goto LEFT_BREAK;
LEFT_BREAK:
	
	//Decrease right
	for( ; right<img.width(); right++ )
		for( int iy=0; iy<img.height(); iy++ )
			if( qAlpha( img.pixel(img.width()-1-right,iy) ) != 0 )
				goto RIGHT_BREAK;
RIGHT_BREAK:
	
	return sub_image( left,top, img.width()-left-right, img.height()-top-bottom );
}


