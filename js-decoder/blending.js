var makeRgba = function( r, g, b, a )
	{ return { r: r, g: g, b: b, a: a }; };

var svgOverlay = function( bottom, top ){
	var normalize = 255;
	return {
			r: (255-top.a) * bottom.r / normalize + top.a * top.r / normalize
		,	g: (255-top.a) * bottom.g / normalize + top.a * top.g / normalize
		,	b: (255-top.a) * bottom.b / normalize + top.a * top.b / normalize
		,	a: 255// - (255-top.a) * (255-bottom.a) / normalize
		};
};

var alphaReplace = function( bottom, top ){
	if( !(top.r == 255 && top.g == 0 && top.b == 255 && top.a == 0 ) )
		return top;
	return bottom;
};

var getRgba = function( data, offset ){
	return {
			r: data[offset+0]
		,	g: data[offset+1]
		,	b: data[offset+2]
		,	a: data[offset+3]
		};
};
var setRgba = function( data, offset, rgba ){
	data[offset+0] = rgba.r;
	data[offset+1] = rgba.g;
	data[offset+2] = rgba.b;
	data[offset+3] = rgba.a;
};

var blendImages = function( img_output, img_overlay, x, y, blender ){
	//TODO: prevent overlay from going out of bounds
	var offset_x = x;
	var offset_y = y;
	var width  = img_overlay.width;
	var height = img_overlay.height;
	
	for( var iy=0; iy<height; iy++ )
		for( var ix=0; ix<width; ix++ ){
			var output_offset = (offset_x+ix)*4 + (offset_y+iy)*img_output.width*4;
			var result = blender(
					getRgba( img_output.data, output_offset )
				,	getRgba( img_overlay.data, ix*4 + iy*img_overlay.width*4 )
				);
			setRgba( img_output.data, output_offset, result );
		}
};