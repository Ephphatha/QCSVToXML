#include "qcsvtoxml.h"

#include <algorithm>

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
	connect(this->ui.lineEditWrapper, &QLineEdit::textChanged, this, &QCSVToXML::refreshXmlPreview);

	connect(this->ui.checkBoxAttributeAsElement, &QCheckBox::toggled, this, &QCSVToXML::refreshXmlPreview);
	connect(this->ui.checkBoxFirstRowAsAttributes, &QCheckBox::toggled, this, &QCSVToXML::refreshXmlPreview);
	connect(this->ui.checkBoxFirstRowAsAttributes, &QCheckBox::toggled, this, &QCSVToXML::prefillAttributeLineEdits);
	connect(this->ui.checkBoxSkipFirstRow, &QCheckBox::toggled, this, &QCSVToXML::refreshXmlPreview);
	connect(this->ui.checkBoxSuppressEmptyAttributes, &QCheckBox::toggled, this, &QCSVToXML::refreshXmlPreview);
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

	this->ui.lineEditElement->setText(fileInfo.baseName());
	this->ui.lineEditRoot->setText(fileInfo.baseName() + "s");

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

	if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
		return;
	}

	QXmlStreamWriter writer(&file);

	this->writeXml(writer);
}

void QCSVToXML::populateAttributeGroupBox()
{
	if (this->csvContents.empty() || this->csvContents[0].size() < this->ui.formLayoutWidgetAttributes->rowCount()) {
		for (int i = 0; i < this->fieldLineEdits.size(); ++i) {
			disconnect(this->fieldLineEdits[i], &QLineEdit::textChanged, this, &QCSVToXML::refreshXmlPreview);
			disconnect(this->ui.checkBoxFirstRowAsAttributes, &QCheckBox::toggled, this->fieldLineEdits[i], &QLineEdit::setDisabled);
		}

		this->fieldLineEdits.clear();

		this->ui.verticalLayoutGroupBoxAttributes->removeWidget(this->ui.widgetAttributes);
		delete this->ui.widgetAttributes;

		this->ui.widgetAttributes = new QWidget(this->ui.groupBoxAttributes);
		this->ui.formLayoutWidgetAttributes = new QFormLayout(this->ui.widgetAttributes);
		this->ui.verticalLayoutGroupBoxAttributes->addWidget(this->ui.widgetAttributes);
	}

	for (int i = this->ui.formLayoutWidgetAttributes->rowCount(); i < this->csvContents[0].size(); ++i) {
		QLineEdit *lineEdit = new QLineEdit(this->ui.widgetAttributes);

		connect(lineEdit, &QLineEdit::textChanged, this, &QCSVToXML::refreshXmlPreview);
		connect(this->ui.checkBoxFirstRowAsAttributes, &QCheckBox::toggled, lineEdit, &QLineEdit::setDisabled);
		lineEdit->setEnabled(!this->ui.checkBoxFirstRowAsAttributes->isChecked());
		this->fieldLineEdits.push_back(lineEdit);
		this->ui.formLayoutWidgetAttributes->addRow(QString("Field %1:").arg(QString::number(i)), lineEdit);
	}

	this->prefillAttributeLineEdits();
}

void QCSVToXML::prefillAttributeLineEdits()
{
	for (int i = 0; i < this->fieldLineEdits.size(); ++i) {
		if (this->ui.checkBoxFirstRowAsAttributes->isChecked() && !this->csvContents.empty() && i < this->csvContents[0].size()) {
			this->fieldLineEdits[i]->setText(this->csvContents[0][i]);
		} else if (this->fieldLineEdits[i]->text().isEmpty()) {
			this->fieldLineEdits[i]->setText(QString("Attribute %1").arg(QString::number(i)));
		}
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

	this->writeXml(writer, true);

	this->ui.textBrowserXmlOutput->setText(xml);
}

void QCSVToXML::writeXml(QXmlStreamWriter &writer, bool preview)
{
	writer.setAutoFormatting(true);

	writer.writeStartDocument();

	if (!this->ui.lineEditNamespaceUri->text().isEmpty()) {
		if (this->ui.lineEditNamespace->text().isEmpty()) {
			writer.writeDefaultNamespace(this->ui.lineEditNamespaceUri->text());
		} else {
			writer.writeNamespace(this->ui.lineEditNamespaceUri->text(), this->ui.lineEditNamespace->text());
		}
	}

	writer.writeStartElement(this->ui.lineEditNamespaceUri->text(), this->ui.lineEditRoot->text().replace(' ', '_'));

	if (!this->ui.lineEditWrapper->text().isEmpty()) {
		writer.writeStartElement(this->ui.lineEditNamespaceUri->text(), this->ui.lineEditWrapper->text().replace(' ', '_'));
	}

	for (int i = this->ui.checkBoxSkipFirstRow->isChecked() ? 1 : 0; i < this->csvContents.size(); ++i) {
		writer.writeStartElement(this->ui.lineEditNamespaceUri->text(), this->ui.lineEditElement->text().replace(' ', '_'));

		for (int j = 0; j < std::min(this->csvContents[i].size(), this->csvContents[0].size()); ++j) {
			if (!this->ui.checkBoxSuppressEmptyAttributes->isChecked() || !this->csvContents[i][j].isEmpty()) {
				if (this->ui.checkBoxAttributeAsElement->isChecked()) {
					writer.writeTextElement(this->ui.lineEditNamespaceUri->text(), this->fieldLineEdits[j]->text().replace(' ', '_'), this->csvContents[i][j]);
				} else {
					writer.writeAttribute(this->ui.lineEditNamespaceUri->text(), this->fieldLineEdits[j]->text().replace(' ', '_'), this->csvContents[i][j]);
				}
			}
		}

		writer.writeEndElement();

		if (preview) {
			break;
		}
	}

	if (!this->ui.lineEditWrapper->text().isEmpty()) {
		writer.writeEndElement();
	}

	writer.writeEndElement();

	writer.writeEndDocument();
}
