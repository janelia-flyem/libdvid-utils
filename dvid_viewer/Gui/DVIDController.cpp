#include "DVIDController.h"
#include "DVIDPlaneController.h"
#include "DVIDUi.h"
#include <QTimer>
#include "../Model/Model.h"
#include "MessageBox.h"
#include <QFileDialog>
#include <QApplication>
#include <algorithm>
#include <sstream>

using std::stringstream;
using std::cout; using std::endl; using std::string;
using std::vector;
using namespace DVIDViewer;

DVIDController::DVIDController(Model* model_, QApplication* qapp_) : 
    model(model_), qapp(qapp_), plane_controller(0)
{
    main_ui = new DVIDUi(model);
    
    // delays loading the views so that the main window is displayed first
    QTimer::singleShot(1000, this, SLOT(load_views()));
    main_ui->showMaximized();
    main_ui->show();

    QObject::connect(main_ui->ui.actionShortcuts, 
            SIGNAL(triggered()), this, SLOT(show_shortcuts()));
    
    QObject::connect(main_ui->ui.actionAbout, 
            SIGNAL(triggered()), this, SLOT(show_about()));
    
    QObject::connect(main_ui->ui.actionHelp, 
            SIGNAL(triggered()), this, SLOT(show_help()));

    QObject::connect(main_ui->ui.actionQuit, 
            SIGNAL(triggered()), this, SLOT(quit_program()));

    QObject::connect(main_ui->ui.planeSlider,
            SIGNAL(valueChanged(int)), main_ui, SLOT(slider_change(int)) );
    
    QObject::connect(main_ui->ui.opacitySlider,
            SIGNAL(valueChanged(int)), main_ui, SLOT(opacity_change(int)) );

    QObject::connect(main_ui->ui.searchLocation, 
            SIGNAL(clicked()), this, SLOT(location_search()));

}

void DVIDController::update()
{
    if (model->get_reset_stack()) {
        int plane_id = 0;    
        model->get_plane(plane_id);
        stringstream str;
        str << model->curr_xloc() << " " << model->curr_yloc() << " " << plane_id;
        main_ui->ui.textEdit->setText(QString::fromStdString(str.str()));
    } 
}

void DVIDController::location_search()
{
    string loc_string = main_ui->ui.textEdit->toPlainText().toStdString();    
    stringstream str(loc_string);
    int x_spot = 0;
    int y_spot = 0;
    int z_spot = 0;
    str >> x_spot >> y_spot >> z_spot;
    
    model->set_location(x_spot, y_spot, z_spot);
}

void DVIDController::quit_program()
{
    // exits the application
    qapp->exit(0);
}

void DVIDController::show_shortcuts()
{
    string msg;
    msg = "Current Key Bindings (not configurable)\n\n";
    msg += "d: increment plane\n";
    msg += "s: decrement plane\n";
    msg += "f: toggle label colors\n";
    msg += "r: empty active body list\n";
    msg += "Up: pan up\n";
    msg += "Down: pan down\n";
    msg += "Left: pan left\n";
    msg += "Right: pan right\n";
    msg += "Left click: select body\n";
    msg += "Shift left click: adds body to active list\n";
    msg += "Scroll: increment/decrement plane\n";
    msg += "Shift+Scroll: zoom in/out\n";
    msg += "t: merge bodies\n";
    msg += "u: next bodies\n";

    MessageBox msgbox(msg.c_str());
}

void DVIDController::show_help()
{
    string msg;

    msg = "Please consult the documentation found in the DVID viewer README";
    msg += " and the README in the examples sub-repository for information";
    msg += " on to use dvid_viewer\n";

    MessageBox msgbox(msg.c_str());
}

void DVIDController::show_about()
{
    string msg;
    
    msg = "The DVID viewer software is a tool currently";
    msg += " being used in the FlyEM project at Janelia Farm Research Campus";
    msg += " to help reconstruct neuronal structure in the fly brain.\n";

    msg += "Stephen Plaza (plaza.stephen@gmail.com)";
    
    MessageBox msgbox(msg.c_str());
}

void DVIDController::clear_session()
{
    if (plane_controller) {
        delete plane_controller;
        plane_controller = 0;
    }
    if (model) {
        main_ui->clear_session();
        model->detach_observer(this);
        delete model;
        model = 0;
    }
}

DVIDController::~DVIDController()
{
    clear_session();
    delete main_ui;
}

void DVIDController::load_views()
{
    if (!model) {
        return;
    }
    model->attach_observer(this);

    // for some reason, I have to call this twice for it to display properly
    main_ui->ui.statusbar->showMessage("Loading Session...");
    main_ui->ui.statusbar->showMessage("Loading Session...");

    plane_controller = new DVIDPlaneController(model, main_ui->ui.planeView);
    plane_controller->initialize();
    plane_controller->start();

    main_ui->ui.clearSelection->disconnect();

    // connect plane controller functionality to the show all labels and clear
    // active label signals

    QObject::connect(main_ui->ui.clearSelection, 
            SIGNAL(clicked()), plane_controller, SLOT(clear_selection()));
    
    main_ui->ui.statusbar->clearMessage();

    model->set_reset_stack();
}
