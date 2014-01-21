#ifndef QCSVTOXML_H
#define QCSVTOXML_H

#include <QtWidgets/QMainWindow>
#include "ui_qcsvtoxml.h"

class QCSVToXML : public QMainWindow
{
	Q_OBJECT

public:
	QCSVToXML(QWidget *parent = 0);
	~QCSVToXML();

private:
	Ui::QCSVToXMLClass ui;
};

#endif // QCSVTOXML_H
