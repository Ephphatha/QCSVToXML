#ifndef QCSVTOXML_H
#define QCSVTOXML_H

#include <QtWidgets/QMainWindow>
#include "ui_qcsvtoxml.h"

#include <vector>
#include <QSettings>

class QCSVToXML : public QMainWindow
{
	Q_OBJECT

public:
	QCSVToXML(QWidget *parent = 0);
	~QCSVToXML();

public slots:
	void loadAction();
	void saveAction();

protected:
	void load(const QString &filename);
	void save(const QString &filename);

	QSettings settings;

	std::vector<std::vector<QString>> csvContents;

protected slots:
	void populateAttributeGroupBox();
	void prefillAttributeLineEdits();
	void refreshXmlPreview();

private:
	Ui::QCSVToXMLClass ui;

	std::vector<QLineEdit *> fieldLineEdits;
};

#endif // QCSVTOXML_H
