//Retrieves a child element were only 1 is expected with this name
var getSingleElement = function( dom, name ){
	var elems = dom.getElementsByTagName( name );
	if( elems.length != 1 )
		throw "There was not exactly 1 instance of '" + name + "' but " + elems.length;
	return elems[0];
}

//Gets an attribute and converts it to a integer, but returns a default value if not specified
var getAttrInt = function( element, name, default_value ){
	if( !element.hasAttribute( name ) )
		return default_value;
	
	var str = element.getAttribute( name );
	var value = parseInt( str );
	if( isNaN( value ) )
		throw "The attribute '" + name + "' must be a number, but was: " + str;
	
	return value;
};

var getChildren = function( dom, name ){
	return [].slice.call( dom.getElementsByTagName( name ) );
};

var stackParser = function( str ){
	var xml = new DOMParser().parseFromString( str, "text/xml" );
	var image = getSingleElement( xml, "image" );
	
	return {
			width:  getAttrInt( image, "w", -1 )
		,	height: getAttrInt( image, "h", -1 )
		,	stacks: getChildren( image, "stack" ).map( function( stack ){
					return getChildren( stack, "layer" ).map( function( layer ){
							return {
									name: layer.getAttribute( "src" )
								,	x: getAttrInt( layer, "x", 0 )
								,	y: getAttrInt( layer, "y", 0 )
								};
						} );
				} )
		};
};

JSZipUtils.getBinaryContent( 'test.cgcompress', function( err, data ){
		if( err ){
			alert( err );
			throw err; // or handle err
		}
		
		//Load zip, just testing with a single WebP image for now
		var zip = new JSZip( data );
		data = zip.files["data/0.webp"].asUint8Array();
		console.log( stackParser( zip.files["stack.xml"].asText() ) );
		
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
		
		//Set up canvas and drawable surface
		var canvas = document.getElementById( "the-canvas" );
		var biHeight = output_buffer.height;
		var biWidth  = output_buffer.width;
		canvas.height = biHeight;
		canvas.width  = biWidth;
		
		var context = canvas.getContext('2d');
		var output = context.createImageData(canvas.width, canvas.height);
		
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
		
		context.putImageData( output, 0, 0 );
		
	} );