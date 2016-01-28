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
								,	overlay: svgOverlay //TODO:
								};
						} );
				} )
		};
};

var loadCgCompress = function( zip ){
	var info = stackParser( zip.files["stack.xml"].asText() );
	return {
			zip: zip
		,	width : info.width
		,	height: info.height
		,	stacks: info.stacks
		
		,	getImage: function( context, name ){
					//TODO: Cache loaded images
					return loadImage( context, this.zip.files[name].asUint8Array() );
				}
		,	drawStack: function( context, stack ){
					var output = context.createImageData( this.width, this.height );
					for( var i=0; i<output.data.length; i++ )
						output.data[i] = 0;
					
					for( var i=stack.length-1; i>=0; i-- ){
						var layer = stack[i];
						blendImages( output, this.getImage( context, layer.name ), layer.x, layer.y, layer.overlay );
					}
					context.putImageData( output, 0, 0 );
				}
		};
};

JSZipUtils.getBinaryContent( 'test5.cgcompress', function( err, data ){
	if( err ){
		alert( err );
		throw err; // or handle err
	}
	
	var cg = loadCgCompress( new JSZip( data ) );
	console.log( cg );
	
	var canvas = document.getElementById( "the-canvas" );
	canvas.width  = cg.width;
	canvas.height = cg.height;
	
	//Just testing with a single WebP image for now
	var context = canvas.getContext( "2d" );
	cg.drawStack( context, cg.stacks[14] );
} );