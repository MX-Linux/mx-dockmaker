#include "picklocation.h"
#include "ui_picklocation.h"

PickLocation::PickLocation(const QString &location, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::PickLocation)
{
    ui->setupUi(this);
    setWindowTitle(tr("Select dock location"));

    buttonGroup = new QButtonGroup(this);
    const QList<QPushButton *> buttons
        = {ui->buttonTL, ui->buttonTC, ui->buttonTR, ui->buttonLC, ui->buttonRC, ui->buttonBL,
           ui->buttonBC, ui->buttonBR, ui->buttonLT, ui->buttonLB, ui->buttonRT, ui->buttonRB};

    const QStringList locations
        = {"TopLeft",      "TopCenter",   "TopRight", "LeftCenter", "RightCenter", "BottomLeft",
           "BottomCenter", "BottomRight", "LeftTop",  "LeftBottom", "RightTop",    "RightBottom"};

    for (int i = 0; i < buttons.size(); ++i) {
        buttonGroup->addButton(buttons.at(i), i + 1);
        buttons[i]->setProperty("location", locations[i]);
    }

    connect(buttonGroup, &QButtonGroup::idClicked, this, &PickLocation::onGroupButton);

    const auto listBtns = buttonGroup->buttons();
    const auto it = std::find_if(listBtns.cbegin(), listBtns.cend(),
                                  [&](const auto *button) {
                                      return location == button->property("location").toString();
                                  });
    if (it != listBtns.cend()) {
        (*it)->click();
    } else {
        ui->buttonBC->click();
    }
}

PickLocation::~PickLocation()
{
    delete ui;
}

void PickLocation::onGroupButton(int button_id)
{
    button = buttonGroup->button(button_id)->property("location").toString();
}
