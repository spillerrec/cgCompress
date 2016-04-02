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

var parseCompositeOp = function( layer ){
	var name = "composite-op"
	if( !layer.hasAttribute( name ) )
		return svgOverlay;
	
	var comp = layer.getAttribute( name );
	switch( comp ){
		case "cgcompress:alpha-replace": return alphaReplace;
		case "svg:src-over": return svgOverlay;
		default: throw "unsupported compositing operation: " + comp;
	};
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
								,	overlay: parseCompositeOp( layer )
								};
						} );
				} )
		};
};

var loadCgCompress = function( data ){
	var zip = new JSZip( data );
	var info = stackParser( zip.files["stack.xml"].asText() );
	return {
			zip: zip
		,	cachedImages: {}
		,	width : info.width
		,	height: info.height
		,	stacks: info.stacks
		
		,	getImage: function( context, name ){
					//Cache image to avoid decoding it each time
					if( this.cachedImages[name] === undefined )
						this.cachedImages[name] = loadImage( context, this.zip.files[name].asUint8Array() );
					return this.cachedImages[name];
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
		,	drawStackUrl: function( stack ){
					var canvas = document.createElement('canvas');
					canvas.width = this.width;
					canvas.height = this.height;
					var context = canvas.getContext('2d');
					this.drawStack( context, stack );
					return canvas.toDataURL();
				}
		};
};

var fetchCgCompressUrl = function( url, callback ){
	JSZipUtils.getBinaryContent( url, function( err, data ){
			if( err )
				throw err;
			callback( loadCgCompress( data ) );
		} );
};
