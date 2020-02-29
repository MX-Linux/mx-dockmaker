#include "picklocation.h"
#include "ui_picklocation.h"

PickLocation::PickLocation(QString location, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PickLocation)
{
    ui->setupUi(this);
    this->setWindowTitle(tr("Select dock location."));

    buttonGroup = new QButtonGroup(this);
    buttonGroup->addButton(ui->buttonTL, 1);
    buttonGroup->addButton(ui->buttonTC, 2);
    buttonGroup->addButton(ui->buttonTR, 3);
    buttonGroup->addButton(ui->buttonLC, 4);
    buttonGroup->addButton(ui->buttonRC, 5);
    buttonGroup->addButton(ui->buttonBL, 6);
    buttonGroup->addButton(ui->buttonBC, 7);
    buttonGroup->addButton(ui->buttonBR, 8);
    buttonGroup->addButton(ui->buttonLT, 9);
    buttonGroup->addButton(ui->buttonLB, 10);
    buttonGroup->addButton(ui->buttonRT, 11);
    buttonGroup->addButton(ui->buttonRB, 12);

    connect(buttonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), this, &PickLocation::onGroupButton);

    bool found = false;
    for (QAbstractButton *button : buttonGroup->buttons()) {
        if (location == button->text()) {
            button->click();
            found = true;
            return;
        }
    }
    if (!found) ui->buttonBC->click();

}

PickLocation::~PickLocation()
{
    delete ui;

}

void PickLocation::onGroupButton(int button_id)
{
    button = buttonGroup->button(button_id)->text();
}

