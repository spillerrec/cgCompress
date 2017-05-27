#include <QCoreApplication>
#include <QFileInfo>
#include <QImage>
#include <QtConcurrent>

#include <QElapsedTimer>

static QImage loadImage( QString path )
	{ return QImage{ path }; }

QStringList splitImage( QString path, int amount ){
	QImage img( path );
	
	auto half = img.size() / 2;
	
	img.copy( 0, 0, half.width(), half.height() ).save( "1.webp", "WEBP", 100 );
	img.copy( half.width(), 0, half.width(), half.height() ).save( "2.webp", "WEBP", 100 );
	img.copy( 0, half.height(), half.width(), half.height() ).save( "3.webp", "WEBP", 100 );
	img.copy( half.width(), half.height(), half.width(), half.height() ).save( "4.webp", "WEBP", 100 );
	
	return QStringList() << "1.webp" << "2.webp" << "3.webp" << "4.webp";
}

int main( int argc, char* argv[] ){
	auto args = QCoreApplication( argc, argv ).arguments();
	args.pop_front();
	
	for( auto& arg : args ){
		QImage img2( arg );
		QElapsedTimer t2;
		t2.start();
		QImage img( arg );
		qDebug() << "Single" << " took " << t2.elapsed() << "ms";
		
		auto paths = splitImage( arg, 4 );
		
		//TODO: time
		QElapsedTimer t;
		t.start();
		auto parts = QtConcurrent::blockingMapped( paths, &loadImage );
		qDebug() << "Multiple" << " took " << t.elapsed() << "ms";
	}
	
	return 0;
}