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

#include "CgImage.hpp"
#include "../images/Blend.hpp"

struct Rect{
   int x, y, width, height;
   
   int right() const{ return x+width; }
   int bottom() const{ return y+height; }
   
   Rect intersection( Rect other ){
      int out_x = std::max( x, other.x );
      int out_y = std::max( y, other.y );
      int out_w = std::min( right(),  other.right()  ) - out_x;
      int out_h = std::min( bottom(), other.bottom() ) - out_y;
      return { out_x, out_y, out_w, out_h };
   }
   
   Rect getCrop( Rect area ){
      return { x-area.x, y-area.y, area.width, area.height };
   }
};


ConstRgbaView CgImage::getPrimitive( int id ){
   return primitives.at(id).img;
}

void CgImage::renderFrameInternal( RgbaView output, int id, int x, int y, int w, int h ){
   auto& info = frames.at( id );
      
   for( auto& blend : info.layers ){
      if( blend.mode == CgBlendType::COPY_FRAME ){
         //Render another frame
         renderFrameInternal( output, blend.primitive_id, blend.x, blend.y, blend.width, blend.height );
      }
      else{
         //Get and crop layer primitive
         auto src = getPrimitive( blend.primitive_id );
         auto src_width  = blend.width  >= 0 ? blend.width  : src.width ();
         auto src_height = blend.height >= 0 ? blend.height : src.height();
         
         auto shared = Rect{ x, y, w, h }.intersection( {blend.x, blend.y, src_width, src_height } );
         
         auto src_crop = Rect{ blend.x, blend.y, blend.width, blend.height }.getCrop( shared );
         
         
         auto layer_src = src.crop( src_crop.x, src_crop.y, src_crop.width, src_crop.height );
         auto layer_out = output.crop( shared.x, shared.y, shared.width, shared.height );
         
         //Blend images
         switch( blend.mode ){
            case CgBlendType::ALPHA_REPLACE:
                  Blending::BlendImages(layer_out, layer_src, &Blending::alphaReplace);
               break;
            
            case CgBlendType::SRC_OVER:
                  Blending::BlendImages(layer_out, layer_src, &Blending::srcOverRgba);
               break;
               
            case CgBlendType::FAST_OVER:
                  Blending::BlendImages(layer_out, layer_src, &Blending::srcOverDirty);
               break;
                  
            case CgBlendType::DIFF_ADD:
                  Blending::BlendImages(layer_out, layer_src, &Blending::applyOffsetRgba);
               break;
               
            case CgBlendType::COPY_FRAME: std::terminate();
         }
      }
   }
}


RgbaImage CgImage::renderFrame( int id ){
   RgbaImage out( width, height, {} );
   renderFrameInternal( out, id, 0, 0, width, height );
   return out;
}

bool CgImage::isValid() const{
   return true; //TODO
}

int CgImage::findPrimitive( const std::string& name ){
   auto it = std::find_if( primitives.begin(), primitives.end(), [&](auto& pri){ return pri.filename == name; } );
   
   if( it == primitives.end() )
      return -1;
   else
      return it - primitives.begin();
}
