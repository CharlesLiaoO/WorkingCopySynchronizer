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
//    QStringList langList;
//    langList<< "System";
//    langList<< "Chinese";
//    langList<< "English";
//    ui->comboBox_Lang->addItems(langList);

    ui->comboBox_Lang->addItem(tr("System"), "System");
    ui->comboBox_Lang->addItem("Chinese(中文)", "Chinese");
    ui->comboBox_Lang->addItem("English", "English");

    IniSetting cfg("config.ini");
    int idx = ui->comboBox_Lang->findData(cfg.value("language", "System").toString());
    ui->comboBox_Lang->setCurrentIndex(idx);
    ui->comboBox_Lang->blockSignals(false);
}

SettingDlg::~SettingDlg()
{
    delete ui;
}

void SettingDlg::on_comboBox_Lang_currentIndexChanged(int index)
{
    IniSetting cfg("config.ini");
    QString sLang = ui->comboBox_Lang->itemData(index).toString();
    cfg.setValue("language", sLang);

    QMessageBox::information(this, "", tr("Take effect after restart"));
    accept();
}

