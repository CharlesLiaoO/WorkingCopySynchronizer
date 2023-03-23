#include "AboutDlg.h"
#include "ui_AboutDlg.h"

#include <QMessageBox>

AboutDlg::AboutDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDlg)
{
    ui->setupUi(this);
}

AboutDlg::~AboutDlg()
{
    delete ui;
}

void AboutDlg::on_toolButton_AboutQt_clicked()
{
    QMessageBox::aboutQt(this);
}

void AboutDlg::on_buttonBox_accepted()
{
    accept();
}

