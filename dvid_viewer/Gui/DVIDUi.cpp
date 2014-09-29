#include "DVIDUi.h"
#include "../Model/Model.h"

using namespace DVIDViewer;

DVIDUi::DVIDUi(Model* model_) : model(model_)
{
    ui.setupUi(this);
    load_session(model);
}

DVIDUi::~DVIDUi()
{
    clear_session();
}

void DVIDUi::clear_session()
{
    if (model) {
        model->detach_observer(this);
        model = 0;
    }
    // buttons disables when no stack session
    ui.actionAddGT->setEnabled(false);
    ui.actionSaveProject->setEnabled(false);
    ui.actionSaveAsProject->setEnabled(false);
    // disable widgets in tool panel
    ui.dockWidget2->setEnabled(false);
}

void DVIDUi::load_session(Model* model_)
{
    if (model) {
        model->detach_observer(this);
    }
    model = model_;
    model->attach_observer(this);

    ui.dockWidget2->setEnabled(true);
    ui.modeWidget->setCurrentIndex(1);

    // get the current plane in the session
    int plane_id = 0;    
    model->get_plane(plane_id);

    // find the max plane (slider is actually oriented as
    // the max_plane - the current plane id 
    max_plane = model->max_plane();
    min_plane = model->min_plane();
    ui.planeSlider->setRange(0, max_plane - min_plane);
    ui.planeSlider->setValue(max_plane - plane_id);

    unsigned int opacity_val = 0;
    model->get_opacity(opacity_val);
    ui.opacitySlider->setValue(opacity_val);
}

void DVIDUi::update()
{
    // update the plane slider position (slider is oriented
    // as the max_plane - the current plane id)
    int plane_id = 0;    
    if (model->get_plane(plane_id) || model->get_reset_stack()) {
        ui.planeSlider->setValue(max_plane - plane_id);
    }

    // update the opacity slider position
    unsigned int opacity_val = 0;
    if (model->get_opacity(opacity_val)) {
        ui.opacitySlider->setValue(opacity_val);
    }
}

void DVIDUi::slider_change(int val)
{
    model->set_plane(max_plane - val);
}

void DVIDUi::opacity_change(int val)
{
    model->set_opacity(val);
}


