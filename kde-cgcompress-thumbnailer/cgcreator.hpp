#ifndef DUMP_CREATOR_HPP
#define DUMP_CREATOR_HPP

#include <kio/thumbcreator.h>

class CgCompressCreator : public ThumbCreator{
	public:
		bool create( const QString& path, int width, int height, QImage& img ) override;
};

#endif
