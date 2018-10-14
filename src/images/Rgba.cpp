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

#include "Rgba.hpp"
#include <QImage>


QImage toQImage( ConstRgbaView image ){
   	QImage qimg( image.width(), image.height(), QImage::Format_ARGB32 );
   	qimg.fill( qRgba( 0,0,0,0 ) );
   	for( int iy=0; iy<image.height(); iy++ ){
   		auto line_in  = image[iy];
   		auto line_out = (QRgb*)qimg.scanLine( iy );
   		for( int ix=0; ix<image.width(); ix++ )
   			line_out[ix] = qRgba( line_in[ix].r, line_in[ix].g, line_in[ix].b, line_in[ix].a );
   	}
   	return qimg;
}

RgbaImage fromQImage( QImage in ){
   QImage qimg = in.convertToFormat(QImage::Format_ARGB32);
   RgbaImage output( qimg.width(), qimg.height() );
   
   for( int iy=0; iy<output.height(); iy++ ){
      auto line_out = output[iy];
      auto line_in  = (const QRgb*)qimg.constScanLine( iy );
      for( int ix=0; ix<output.width(); ix++ )
         line_out[ix] = Rgba( qRed(line_in[ix]), qGreen(line_in[ix]), qBlue(line_in[ix]), qAlpha(line_in[ix]) );
   }
   
   return output;
}


void removeAlpha( RgbaView image ){
   for( auto line : image )
      for( auto& pixel : line )
         pixel.a = 255;
}
void removeTransparent( RgbaView image, Rgba clear_color ){
   for( auto line : image )
      for( auto& pixel : line )
         if( pixel.a == 0 )
            pixel = clear_color;
}