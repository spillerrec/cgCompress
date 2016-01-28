var loadImage = function( context, data ){
	//All the weird decoding code which looks like it was ported from C
	var WebPImage = { width:{value:0},height:{value:0} };
	var decoder = new WebPDecoder();
	
	var config = decoder.WebPDecoderConfig;
	var output_buffer = config.j;
	
	if( !decoder.WebPInitDecoderConfig(config) ){
		alert( "Library version mismatch!\n" ); //How the fuck can this happen in JavaScript?
		return -1;
	}
	
	var StatusCode = decoder.VP8StatusCode;
	
	status = decoder.WebPGetFeatures( data, data.length, config.input );
	if( status != 0 ){
		alert( 'error' );
		return -1;
	}
	
	output_buffer.J = 4; //Great naming right here, especially since it is config.j.J
	
	status = decoder.WebPDecode( data, data.length, config );
	
	if( status != 0 ){
		alert( "Decoding failed" );
		return -1;
	}
	
	//Set up drawable surface
	var biHeight = output_buffer.height;
	var biWidth  = output_buffer.width;
	var output = context.createImageData(biWidth, biHeight);
	
	//Convert from RGBA to ARGB
	var bitmap = output_buffer.c.RGBA.ma;
	var outputData = output.data;
	for( var h=0; h<biHeight; h++ ){
		for( var w=0; w<biWidth; w++ ){
			outputData[0+w*4+(biWidth*4)*h] = bitmap[1+w*4+(biWidth*4)*h];
			outputData[1+w*4+(biWidth*4)*h] = bitmap[2+w*4+(biWidth*4)*h];
			outputData[2+w*4+(biWidth*4)*h] = bitmap[3+w*4+(biWidth*4)*h];
			outputData[3+w*4+(biWidth*4)*h] = bitmap[0+w*4+(biWidth*4)*h];
		}
	}
	
	return output;
};