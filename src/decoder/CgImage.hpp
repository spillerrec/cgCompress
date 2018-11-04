/*
	This file is part of qt5-ora-plugin.

	qt5-ora-plugin is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	qt5-ora-plugin is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with qt5-ora-plugin.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CG_IMAGE_HPP
#define CG_IMAGE_HPP

#include <vector>
#include <string>
#include "../images/Rgba.hpp"

enum class CgBlendType{
   SRC_OVER,
   ALPHA_REPLACE,
   FAST_OVER,
   DIFF_ADD,
   COPY_FRAME //Refers to a CgFrame, not a primitive
};

struct CgBlend{
   size_t primitive_id;
   int x      {  0 };
   int y      {  0 };
   int width  { -1 }; //Negative means full width
   int height { -1 }; //Negative means full height
   CgBlendType mode;
};

struct CgPrimitive{
   RgbaImage img;
   std::string filename; //NOTE: UTF-8 encoded
   //TODO: Support some form for lazy loading
};

struct CgFrame{
   std::vector<CgBlend> layers;
   std::string name;//NOTE: UTF-8 encoded
   //TODO: Other metadata
};

class CgImage{
   private:
      std::vector<CgPrimitive> primitives;
      std::vector<CgFrame> frames;
      int width{0}, height{0}; //Image dimensions
   
      ConstRgbaView getPrimitive( int id );
      void renderFrameInternal( RgbaView output, int id, int x, int y, int w, int h );
      
   public:
      int frameCount() const{ return frames.size(); }
      RgbaImage renderFrame( int id );
      
      void addPrimitive( CgPrimitive&& pri ){ primitives.push_back(std::move(pri)); }
      void addFrame( CgFrame frame ){ frames.push_back( frame ); }
      void setDimensions( int width, int height ){
         this->width  = width;
         this->height = height;
      }
      
      bool isValid() const;
      
      int findPrimitive( const std::string& name );
};

#endif
