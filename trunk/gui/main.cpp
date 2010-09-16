#include <qapplication.h>
#include "canvas_qt4.h"


int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	MainForm* window = new MainForm;
	if (argc > 1) // when started from command line with mezzo filename
	{
		window->set_started_from_commandline(true);
		if (argc > 2) // if random seed is given, set before reading and initialising network
		{
			window->seed(atoi (argv[2]));		
		}
		window->set_filename(QString (argv[1])); // open master file and process, open all other files.
		window->show();
		window->auto_run(); // automatically run and save results
		return 0;
	}
	else
		window->show();
	return app.exec();
}