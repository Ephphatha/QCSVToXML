#include "qcsvtoxml.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QCSVToXML w;
	w.show();
	return a.exec();
}
