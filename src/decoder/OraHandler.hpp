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

#ifndef ORA_DECODER_HPP
#define ORA_DECODER_HPP

#include <map>

#include <QIODevice>

#include <archive.h>

#include <pugixml.hpp>

#include "../images/Rgba.hpp"

class OraDecoder{
	private:
		int frame{ -1 };
		bool loaded{ false };
		
		std::map<QString,RgbaImage> images;
		
		pugi::xml_document doc;
		
		bool read_and_validate( archive *a );
		
		void render_stack( pugi::xml_node node, RgbaImage &output, int offset_x=0, int offset_y=0 ) const;
	
	public:
		bool load(QIODevice& device);
		
		RgbaImage read();
		
		int imageCount() const;
		
		
		static void alpha_replace( RgbaView output, ConstRgbaView image, int dx, int dy );
		static void src_over(      RgbaView output, ConstRgbaView image, int dx, int dy );
		
};


#endif
