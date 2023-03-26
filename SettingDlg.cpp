#include "SettingDlg.h"
#include "ui_SettingDlg.h"

#include <NotPrjRel.h>
#include <QMessageBox>

SettingDlg::SettingDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingDlg)
{
    ui->setupUi(this);

    ui->label_LangIcon->setMinimumSize(20, 20);
    ui->label_LangIcon->setPixmap(QPixmap("://images/locale.png").scaledToHeight(20));

    ui->comboBox_Lang->blockSignals(true);

    ui->comboBox_Lang->addItem(tr("System"), "System");
    ui->comboBox_Lang->addItem("Chinese(中文)", "Chinese");
    ui->comboBox_Lang->addItem("English", "English");

    IniSetting setting("setting.ini");
    int idx = ui->comboBox_Lang->findData(setting.value("language", "System").toString());
    ui->comboBox_Lang->setCurrentIndex(idx);

    ui->comboBox_Lang->blockSignals(false);
}

SettingDlg::~SettingDlg()
{
    delete ui;
}

void SettingDlg::on_comboBox_Lang_currentIndexChanged(int index)
{
    IniSetting setting("setting.ini");
    QString sLang = ui->comboBox_Lang->itemData(index).toString();
    setting.setValue("language", sLang);

    QMessageBox::information(this, "", tr("Take effect after restart"));
    accept();
}

