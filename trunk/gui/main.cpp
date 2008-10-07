#include <qapplication.h>
#include "canvas_qt4.h"


int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	MainForm* window = new MainForm;
	if (argc > 1)
	{
		if (argc > 2) // if random seed is given, set before reading and initialising network
		{
			window->seed(atoi (argv[2]));
		}
		window->set_filename(QString (argv[1]));
	}
	window->show();
	return app.exec();
}