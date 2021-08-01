#include "picklocation.h"
#include "ui_picklocation.h"

PickLocation::PickLocation(QString location, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PickLocation)
{
    ui->setupUi(this);
    this->setWindowTitle(tr("Select dock location"));

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

    ui->buttonTL->setProperty("location", "TopLeft");
    ui->buttonTC->setProperty("location", "TopCenter");
    ui->buttonTR->setProperty("location", "TopRight");
    ui->buttonLC->setProperty("location", "LeftCenter");
    ui->buttonRC->setProperty("location", "RightCenter");
    ui->buttonBL->setProperty("location", "BottomLeft");
    ui->buttonBC->setProperty("location", "BottomCenter");
    ui->buttonBR->setProperty("location", "BottomRight");
    ui->buttonLT->setProperty("location", "LeftTop");
    ui->buttonLB->setProperty("location", "LeftBottom");
    ui->buttonRT->setProperty("location", "RightTop");
    ui->buttonRB->setProperty("location", "RightBottom");

    connect(buttonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), this, &PickLocation::onGroupButton);

    bool found = false;
    for (auto *button : buttonGroup->buttons()) {
        if (location == button->property("location").toString()) {
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
    button = buttonGroup->button(button_id)->property("location").toString();
}

