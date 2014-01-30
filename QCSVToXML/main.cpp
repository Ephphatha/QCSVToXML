#include "qcsvtoxml.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QCoreApplication::setApplicationName("QCSVToXML");
	QCoreApplication::setOrganizationName("The Letter Eph");
	QCoreApplication::setOrganizationDomain("thelettereph.com");
	QCSVToXML w;
	w.show();
	return a.exec();
}
