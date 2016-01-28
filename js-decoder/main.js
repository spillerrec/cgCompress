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

var drawStack = function( context, stack, zip, info ){ //TODO: avoid zip and info
	var output = context.createImageData( info.width, info.height );
	for( var i=0; i<output.data.length; i++ )
		output.data[i] = 0;
	
	for( var i=stack.length-1; i>=0; i-- ){
		var layer = stack[i];
		var data2 = zip.files[layer.name].asUint8Array();
		blendImages( output, loadImage( context, data2 ), layer.x, layer.y, svgOverlay );
	}
	context.putImageData( output, 0, 0 );
};

JSZipUtils.getBinaryContent( 'test.cgcompress', function( err, data ){
		if( err ){
			alert( err );
			throw err; // or handle err
		}
		
		//Load zip, just testing with a single WebP image for now
		var zip = new JSZip( data );
		var info = stackParser( zip.files["stack.xml"].asText() );
		console.log( info );
		
		var canvas = document.getElementById( "the-canvas" );
		canvas.width  = info.width;
		canvas.height = info.height;
		
		var context = canvas.getContext( "2d" );
		
		drawStack( context, info.stacks[1], zip, info );
	} );