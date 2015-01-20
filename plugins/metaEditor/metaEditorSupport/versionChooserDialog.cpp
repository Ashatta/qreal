#include "versionChooserDialog.h"
#include "ui_versionchooserdialog.h"

VersionChooserDialog::VersionChooserDialog(QMap<int, QString> const &versions, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::VersionChooserDialog)
{
	ui->setupUi(this);

	for (int version : versions.keys()) {
		ui->fromComboBox->addItem(versions[version], version);
		ui->toComboBox->addItem(versions[version], version);
	}

	connect(this, SIGNAL(accepted()), this, SLOT(onAccept()));
}

VersionChooserDialog::~VersionChooserDialog()
{
	delete ui;
}

void VersionChooserDialog::onAccept()
{
	emit versionChosen(ui->fromComboBox->currentData().toInt(), ui->toComboBox->currentData().toInt());
}
