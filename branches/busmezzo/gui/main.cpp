#include <qapplication.h>
#include "canvas_qt4.h"


int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	MainForm* window = new MainForm;
	window->show();
	return app.exec();
}