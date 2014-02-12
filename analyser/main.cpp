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

#include <QMap>
#include <QImage>

#include <vector>
#include <algorithm>

int hue( QRgb color ){
	int min = std::min( qRed(color), std::min( qGreen(color), qBlue(color) ) );
	int max = std::max( qRed(color), std::max( qGreen(color), qBlue(color) ) );
	
	int delta = max - min;
	if( delta == 0 )
		return 0; //TODO: something!
	
	int saturation = delta  / (double)max;
	
	double hue;
	if( qRed(color) == max )
		hue = ( qGreen(color) - qBlue(color) ) / (double)delta;
	else if( qGreen(color) == max )
		hue = 2 + ( qBlue(color) - qRed(color) ) / (double)delta;
	else
		hue = 4 + ( qRed(color) - qGreen(color) ) / (double)delta;
	
	hue *= 60;
	if( hue < 0 )
		hue += 360;
	return hue;
}
int saturation( QRgb color ){
	int min = std::min( qRed(color), std::min( qGreen(color), qBlue(color) ) );
	int max = std::max( qRed(color), std::max( qGreen(color), qBlue(color) ) );
	
	int delta = max - min;
	if( delta == 0 )
		return 0; //TODO: something!
	
	return delta  / (double)max * 255;
}

std::vector<int> calculate_histogram( QImage image, int f(QRgb), int width=256 ){
	std::vector<int> histo( width, 0 );
	for( int iy=0; iy<image.height(); iy++ ){
		const QRgb* input = (const QRgb*)image.constScanLine( iy );
		for( int ix=0; ix<image.width(); ix++ )
			histo[ f( input[ix] ) ]++;
	}
	return histo;
}

int change_add( int org, int change ){
	return org + change;
}
int change_max( int org, int change ){
	return std::max( org, change );
}
int change_min( int org, int change ){
	return std::min( org, change );
}

std::vector<int> calculate_changes( QImage input, QImage output, int f(QRgb), int width=256, int transfer(int,int)=&change_add ){
	if( input.size() != output.size() )
		qFatal( "Images do not have the same dimensions!" );
		
	std::vector<int> changes( width, 0 );
	for( int iy=0; iy<input.height(); iy++ ){
		const QRgb* in = (const QRgb*)input.constScanLine( iy );
		const QRgb* out = (const QRgb*)output.constScanLine( iy );
		for( int ix=0; ix<input.width(); ix++ )
			changes[ f( in[ix] ) ] = transfer( changes[ f( in[ix] ) ], f( in[ix] ) - f( out[ix] ) );
	}
	return changes;
}

QImage make_histogram( QImage image, int f(QRgb), int width=256 ){
	QImage output( width, 300, QImage::Format_RGB32 );
	output.fill( qRgb( 255,255,255 ) );
	
	std::vector<int> histo( calculate_histogram( image, f, width ) );
	double maximum = *std::max_element( histo.begin(), histo.end() );
	
	for( int ix=0; ix<width; ix++ ){
		int height = histo[ix]*output.height() / maximum;
		for( int iy=0; iy<height; iy++ )
			output.setPixel( ix, output.height()-iy-1, 0 );
	}
	
	return output;
}

QImage make_histogram3( QImage image, int r(QRgb), int g(QRgb), int b(QRgb), int width=256 ){
	QImage output( width, 300, QImage::Format_RGB32 );
	output.fill( qRgb( 0,0,0 ) );
	
	std::vector<int> histo_r( calculate_histogram( image, r, width ) );
	std::vector<int> histo_g( calculate_histogram( image, g, width ) );
	std::vector<int> histo_b( calculate_histogram( image, b, width ) );
	
	auto maximum_r = *std::max_element( histo_r.begin(), histo_r.end() );
	auto maximum_g = *std::max_element( histo_g.begin(), histo_g.end() );
	auto maximum_b = *std::max_element( histo_b.begin(), histo_b.end() );
	double maximum = std::max( maximum_r, std::max( maximum_g, maximum_b ) );
	
	for( int ix=0; ix<width; ix++ ){
		int height_r = histo_r[ix]*output.height() / maximum;
		int height_g = histo_g[ix]*output.height() / maximum;
		int height_b = histo_b[ix]*output.height() / maximum;
		
		int height = std::max( height_r, std::max( height_g, height_b ) );
		for( int iy=0; iy<height; iy++ )
			output.setPixel( ix, output.height()-iy-1, qRgb(
					iy<height_r ? 255 : 0
				,	iy<height_g ? 255 : 0
				,	iy<height_b ? 255 : 0
				) );
	}
	
	return output;
}

QImage make_changes( QImage original, QImage transformed, int f(QRgb), int width=256 ){
	auto histo = calculate_histogram( original, f, width );
	auto changes = calculate_changes( original, transformed, f, width );
	auto mins = calculate_changes( original, transformed, f, width, &change_min );
	auto maxs = calculate_changes( original, transformed, f, width, &change_max );
	auto maximum = *std::max_element( histo.begin(), histo.end() );
	
	QImage output( width, width*2, QImage::Format_RGB32 );
	output.fill( qRgb( 255,255,255 ) );
	
	for( int ix=0; ix<width; ix++ )
		output.setPixel( ix, width, qRgb( 127,127,127 ) );
	
	for( int ix=0; ix<width; ix++ ){
		if( histo[ix] != 0 ){
			int change = width + changes[ix] / histo[ix];
			int value = 0;//255 - 255.0 * histo[ix] / maximum;
			output.setPixel( ix, change, qRgb( value, value, value ) );
			output.setPixel( ix, width + mins[ix], qRgb( 127, 127, 127 ) );
			output.setPixel( ix, width + maxs[ix], qRgb( 127, 127, 127 ) );
		}
	}
	
	return output;
}
	

void test_consistency( QImage image, QImage transformed ){

	QMap<int,int> transform;
	long errors = 0;
	long dupes = 0;
	
	QList<int> error_red;
	for( int i=0; i<256; i++ )
		error_red.append( 0 );
	QList<int> error_green = error_red;
	QList<int> error_blue = error_red;
	
	QImage output( image.width(), image.height(), QImage::Format_ARGB32 );
	output.fill( qRgb( 127, 127, 127 ) );
	
	for( int iy=0; iy<image.height(); iy++ ){
		QRgb* org = (QRgb*)image.scanLine( iy );
		QRgb* result = (QRgb*)transformed.scanLine( iy );
		QRgb* out = (QRgb*)output.scanLine( iy );
		
		for( int ix=0; ix<image.width(); ix++ ){
			int from = org[ix];
			int to = result[ix];
			if( transform.contains( from ) ){
				dupes++;
				if( transform[from] != to ){
					errors++;
					out[ix] = qRgb(
							127 + qRed(to) - qRed(transform[from])
						,	127 + qGreen(to) - qGreen(transform[from])
						,	127 + qBlue(to) - qBlue(transform[from])
						);
					error_red[abs( qRed(to) - qRed(transform[from]) )]++;
					error_green[abs( qGreen(to) - qGreen(transform[from]) )]++;
					error_blue[abs( qBlue(to) - qBlue(transform[from]) )]++;
				}
			}
			else{
				transform[from] = to;
				out[ix] = qRgb( 127, 127, 127 );
			}
		}
	}
	output.save( "output.png" );
	
	qDebug( "correct: %ld", dupes - errors );
	qDebug( "wrong: %ld", errors );
	qDebug( "reused colors: %f", dupes*100.0 / (image.width() * image.height() ) );
	
	qDebug( "transforms: %d", transform.size() );
	
	for( int i=0; i<256; i++ )
		if( error_red[i] != 0 || error_green[i] != 0 || error_blue[i] != 0 )
			qDebug( "%d - %d - %d - %d", i, error_red[i], error_green[i], error_blue[i] );
	
}

#include <utility>
#include <cmath>
using namespace std;
int diff_center( int from, int to, int offset=127 ){
	return offset + from - to;;
}

int diff_wrap( int from, int to ){
	int diff = from - to;
	if( diff > 0 )
		return diff;
	else
		return 255 - diff;
}

pair<int,int> diff_range( QImage image, QImage transformed, int f(QRgb) ){
	QImage output( image.width(), image.height(), QImage::Format_ARGB32 );
	
	int max_val = INT_MIN;
	int min_val = INT_MAX;
	
	for( int iy=0; iy<image.height(); iy++ ){
		QRgb* org = (QRgb*)image.scanLine( iy );
		QRgb* result = (QRgb*)transformed.scanLine( iy );
		
		for( int ix=0; ix<image.width(); ix++ ){
			int val = diff_center( f( org[ix] ), f( result[ix] ), 0 );
			max_val = max( max_val, val );
			min_val = min( min_val, val );
		}
	}
	
	return make_pair( min_val, max_val );
}

int diff_offset( pair<int,int> range ){
	qDebug( "Range: %d, %d", range.first, range.second );
	qDebug( "Width: %d", range.second - range.first );
	
	int offset = max( -range.first, 0 );
	qDebug( "Offset %d", offset );
	return offset;
}

void output_diff( QImage image, QImage transformed ){
	QImage output( image.width(), image.height(), QImage::Format_ARGB32 );
	
	int red_offset = diff_offset( diff_range( image, transformed, &qRed ) );
	int green_offset = diff_offset( diff_range( image, transformed, &qGreen ) );
	int blue_offset = diff_offset( diff_range( image, transformed, &qBlue ) );
	
	for( int iy=0; iy<image.height(); iy++ ){
		QRgb* org = (QRgb*)image.scanLine( iy );
		QRgb* result = (QRgb*)transformed.scanLine( iy );
		QRgb* out = (QRgb*)output.scanLine( iy );
		
		for( int ix=0; ix<image.width(); ix++ ){
			out[ix] = qRgb(
					diff_center( qRed(org[ix]), qRed(result[ix]), red_offset )
				,	diff_center( qGreen(org[ix]), qGreen(result[ix]), green_offset )
				,	diff_center( qBlue(org[ix]), qBlue(result[ix]), blue_offset )
				);
		}
	}
	output.save( "diff_ranged.webp", nullptr, 100 );
}


int main( int argc, char* argv[] ){
	QCoreApplication app( argc, argv );
	
	QStringList args = app.arguments();
	args.removeFirst();
	
	if( args.size() != 2 )
		qFatal( "Must get 2 images!" );
	
	QImage image( args[0] );
	QImage transformed( args[1] );
	
	if( image.width() != transformed.width()
		|| image.height() != transformed.height() )
		qFatal( "Images must have same dimensions" );
	
//	test_consistency( image, transformed );
	output_diff( image, transformed );
	
	make_changes( image, transformed, &qGray ).save( "change-gray.png" );
	make_changes( image, transformed, &saturation ).save( "change-saturation.png" );
	make_changes( image, transformed, &qRed ).save( "change-red.png" );
	make_changes( image, transformed, &qGreen ).save( "change-green.png" );
	make_changes( image, transformed, &qBlue ).save( "change-blue.png" );
	make_changes( image, transformed, &hue, 360 ).save( "change-hue.png" );
	
	make_histogram( image, &hue, 360 ).save( "hue-image.png" );
	make_histogram( image, &qGray ).save( "gray-image.png" );
	make_histogram3( image, &qRed, &qGreen, &qBlue ).save( "rgb-image-rgb.png" );
	
	make_histogram( transformed, &hue, 360 ).save( "hue-transformed.png" );
	make_histogram( transformed, &qGray ).save( "gray-transformed.png" );
	make_histogram3( transformed, &qRed, &qGreen, &qBlue ).save( "rgb-transformed.png" );
	
	return 0;
}