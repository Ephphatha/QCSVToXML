#include "qcsvtoxml.h"

#include <QFileDialog>
#include <QTextStream>
#include <QXmlStreamWriter>

QCSVToXML::QCSVToXML(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	connect(this->ui.actionLoad, &QAction::triggered, this, &QCSVToXML::loadAction);
	connect(this->ui.actionExport, &QAction::triggered, this, &QCSVToXML::saveAction);

	connect(this->ui.lineEditElement, &QLineEdit::textChanged, this, &QCSVToXML::refreshXmlPreview);
	connect(this->ui.lineEditNamespace, &QLineEdit::textChanged, this, &QCSVToXML::refreshXmlPreview);
	connect(this->ui.lineEditNamespaceUri, &QLineEdit::textChanged, this, &QCSVToXML::refreshXmlPreview);
	connect(this->ui.lineEditRoot, &QLineEdit::textChanged, this, &QCSVToXML::refreshXmlPreview);

	connect(this->ui.checkBoxAttributeAsElement, &QCheckBox::toggled, this, &QCSVToXML::refreshXmlPreview);
	connect(this->ui.checkBoxFirstRowAsAttributes, &QCheckBox::toggled, this, &QCSVToXML::refreshXmlPreview);
}

QCSVToXML::~QCSVToXML()
{

}

void QCSVToXML::loadAction()
{
	this->load(
		QFileDialog::getOpenFileName(
			this,
			tr("Load File"),
			this->settings.value(
				"workingDirectory",
				QApplication::applicationDirPath()).toString(),
			tr("CSV files (*.csv)")));
}

void QCSVToXML::saveAction()
{
	this->save(
		QFileDialog::getSaveFileName(
			this,
			tr("Save File"),
			this->settings.value(
				"workingDirectory",
				QApplication::applicationDirPath()).toString(),
			tr("XML files (*.xml)")));
}

void QCSVToXML::load(const QString &filename)
{
	if (filename.isEmpty()) {
		return;
	}

	QFile file(filename);
	QFileInfo fileInfo(file);

	this->settings.setValue("workingDirectory", fileInfo.absolutePath());

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return;
	}

	this->csvContents.clear();

	QTextStream in(&file);

	while (!in.atEnd()) {
		QString line = in.readLine();
		while (line.count('"') % 2 != 0 && !in.atEnd()) {
			// If we have an odd number of quotes there must be a newline in
			//  one of the fields. (or RFC 4180 is not being followed, in which
			//  case we don't care about handling the file properly.)
			line += '\n' + in.readLine();
		}

		bool quoted = false;
		int start = 0;

		std::vector<QString> elements;

		for (int i = 0; i <= line.length(); ++i) {
			if (i == line.length()) {
				elements.push_back(line.mid(start));
			} else if (line[i] == '"') {
				quoted = !quoted;
			} else if (line[i] == ',' && !quoted) {
				elements.push_back(line.mid(start, i - start));
				start = i + 1;
			}
		}

		for (int i = 0; i < elements.size(); ++i) {
			// remove quotes surrounding quoted fields.
			elements[i].replace(
				QRegularExpression(
					"^\\s*\"(.*)\"\\s*$",
					QRegularExpression::DotMatchesEverythingOption),
				"\\1");

			// unescape quotes.
			elements[i].replace("\"\"", "\"");
		}

		this->csvContents.push_back(elements);
	}

	this->populateAttributeGroupBox();
	this->refreshXmlPreview();
}

void QCSVToXML::save(const QString &filename)
{
	if (filename.isEmpty()) {
		return;
	}

	QFile file(filename);
	QFileInfo fileInfo(file);

	this->settings.setValue("workingDirectory", fileInfo.absolutePath());
}

void QCSVToXML::populateAttributeGroupBox()
{
	if (this->csvContents.empty() || this->csvContents[0].size() < this->ui.formLayoutWidgetAttributes->rowCount()) {
		this->ui.verticalLayoutGroupBoxAttributes->removeWidget(this->ui.widgetAttributes);
		delete this->ui.widgetAttributes;
		this->ui.widgetAttributes = new QWidget(this->ui.groupBoxAttributes);
		this->ui.formLayoutWidgetAttributes = new QFormLayout(this->ui.widgetAttributes);
		this->ui.verticalLayoutGroupBoxAttributes->addWidget(this->ui.widgetAttributes);
	}

	for (int i = this->ui.formLayoutWidgetAttributes->rowCount(); i < this->csvContents[0].size(); ++i) {
		this->ui.formLayoutWidgetAttributes->addRow(QString("Field %1").arg(QString::number(i)), new QLineEdit(this->ui.widgetAttributes));
	}
}

void QCSVToXML::refreshXmlPreview()
{
	if (this->csvContents.empty()) {
		this->ui.textBrowserXmlOutput->clear();
		return;
	}

	QString xml;

	QXmlStreamWriter writer(&xml);

	writer.setAutoFormatting(true);

	writer.writeStartDocument();
	writer.writeStartElement("element");

	for (int i = 0; i < this->csvContents[0].size(); ++i) {
		writer.writeTextElement(QString("Attribute %1").arg(QString::number(i)), this->csvContents[0][i]);
	}

	writer.writeEndElement();
	writer.writeEndDocument();

	this->ui.textBrowserXmlOutput->setText(xml);
}
