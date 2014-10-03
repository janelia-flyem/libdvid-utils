#include "DVIDPlaneController.h"
#include "DVIDPlaneView.h"
#include "../Model/Model.h"
#include <vtkPointData.h>
#include <vtkRenderer.h>
#include <vtkAssemblyPath.h>
#include <vtkImageActor.h>
#include "QVTKWidget.h"
#include <QtGui/QWidget>

using namespace DVIDViewer;
using std::string;

vtkClickCallback *vtkClickCallback::New() 
{
    return new vtkClickCallback();
}

void vtkClickCallback::Execute(vtkObject *caller, unsigned long, void*)
{
    vtkRenderWindowInteractor *iren = 
        reinterpret_cast<vtkRenderWindowInteractor*>(caller);
    char * sym_key = iren->GetKeySym();
    string key_val = "";
    if (sym_key) {
        key_val = string(sym_key);
    }

    vtkRenderer* renderer = image_viewer->GetRenderer();
    vtkImageActor* actor = image_viewer->GetImageActor();
    vtkInteractorStyle *style = vtkInteractorStyle::SafeDownCast(
            iren->GetInteractorStyle());

    // Pick at the mouse location provided by the interactor
    prop_picker->Pick( iren->GetEventPosition()[0],
            iren->GetEventPosition()[1],
            0.0, renderer );

    // There could be other props assigned to this picker, so 
    // make sure we picked the image actor
    vtkAssemblyPath* path = prop_picker->GetPath();
    bool validPick = false;

    if (path) {
        vtkCollectionSimpleIterator sit;
        path->InitTraversal(sit);
        vtkAssemblyNode *node;
        for(int i = 0; i < path->GetNumberOfItems() && !validPick; ++i)
        {
            node = path->GetNextNode( sit );
            if( actor == vtkImageActor::SafeDownCast( node->GetViewProp() ) )
            {
                validPick = true;
            }
        }
    }

    if ( !validPick ) {
        return;
    }
    double pos[3];
    prop_picker->GetPickPosition( pos );

    if (iren->GetShiftKey() && enable_selection) {
        // select label and add to active list
        model->active_label(int(pos[0]),
                model->shape(1) - int(pos[1]) - 1, int(pos[2]));
    } else {
        // toggle color for a given label
        model->select_label(int(pos[0]),
                model->shape(1) - int(pos[1]) - 1, int(pos[2]));
    }
}

vtkClickCallback::vtkClickCallback() : model(0), enable_selection(true) 
{
    PointData = vtkPointData::New();
}


void vtkClickCallback::set_image_viewer(vtkSmartPointer<vtkImageViewer2> image_viewer_)
{
    image_viewer = image_viewer_;
}

void vtkClickCallback::set_picker(vtkSmartPointer<vtkPropPicker> prop_picker_)
{
    prop_picker = prop_picker_;
}

void vtkClickCallback::set_model(Model* model_)
{
    model = model_;
}

vtkClickCallback::~vtkClickCallback()
{
    PointData->Delete();
}

void vtkSimpInteractor::OnKeyPress()
{
    //EnabledOff();
    vtkRenderWindowInteractor *iren = this->Interactor;

    string key_val = string(iren->GetKeySym());
    //cout << key_val << endl;

    if (key_val == "d") {
        // increase plane by going down stack
        model->increment_plane();
    } else if (key_val == "s") {
        // decrease plane by going down stack
        model->decrement_plane();
    } else if (key_val == "f") {
        // toggle between grayscale and color
        model->toggle_show_all();    
    } else if ((key_val == "r") && enable_selection) {
        // remove all of the selected labels
        model->reset_active_labels();    
    } else if (key_val == "Up") {
        model->pan(0, -1);
    } else if (key_val == "Down") {
        model->pan(0, 1);
    } else if (key_val == "Left") {
        model->pan(-1, 0);
    } else if (key_val == "Right") {
        model->pan(1, 0);
    } 
   // EnabledOn();
}

void vtkSimpInteractor::OnMouseWheelBackward()
{
    vtkRenderWindowInteractor *iren = this->Interactor;
    char * sym_key = iren->GetKeySym();
    string key_val = "";
    if (sym_key) {
        key_val = string(sym_key);
    }

    if (key_val == "Shift_L" || key_val == "Shift_R") {
        vtkInteractorStyleImage::OnMouseWheelBackward(); 
    } else {
        model->decrement_plane();
    }
}

void vtkSimpInteractor::OnMouseWheelForward()
{
    vtkRenderWindowInteractor *iren = this->Interactor;
    char * sym_key = iren->GetKeySym();
    string key_val = "";
    if (sym_key) {
        key_val = string(sym_key);
    }
    
    if (key_val == "Shift_L" || key_val == "Shift_R") {
        vtkInteractorStyleImage::OnMouseWheelForward(); 
    } else {
        model->increment_plane();
    }
}

void vtkSimpInteractor::OnKeyRelease()
{
    vtkRenderWindowInteractor *iren = this->Interactor;
    iren->SetKeySym(0); 
}
void vtkSimpInteractor::set_model(Model* model_)
{
    model = model_;
}
    
void vtkSimpInteractor::set_view(DVIDPlaneView* view_)
{
    view = view_;
}

void DVIDPlaneController::toggle_show_all()
{
    model->toggle_show_all();
}

void DVIDPlaneController::clear_selection()
{
    model->reset_active_labels();    
}

DVIDPlaneController::DVIDPlaneController(Model* model_,
            QWidget* widget_parent) : model(model_), view(0)
{
    model->attach_observer(this);
    view = new DVIDPlaneView(model, this, widget_parent);
}

void DVIDPlaneController::enable_selections()
{
    // in some modes, enable the ability to select bodies
    click_callback->enable_selections();
    interactor_style->enable_selections();
}

void DVIDPlaneController::disable_selections()
{
    // in some modes, disable the ability to select bodies
    click_callback->disable_selections();
    interactor_style->disable_selections();
}

DVIDPlaneController::~DVIDPlaneController()
{
    delete view;
}

void DVIDPlaneController::initialize()
{
    view->initialize();

    vtkSmartPointer<vtkImageViewer2> image_viewer = view->get_viewer();
    vtkSmartPointer<vtkPropPicker> prop_picker = vtkSmartPointer<vtkPropPicker>::New();
    prop_picker->PickFromListOn();
    prop_picker->AddPickList(image_viewer->GetImageActor());

    click_callback = vtkSmartPointer<vtkClickCallback>::New();
    click_callback->set_model(model);
    click_callback->set_image_viewer(image_viewer);
    click_callback->set_picker(prop_picker);

    QVTKInteractor* interactor = view->get_interactor(); 
    interactor->AddObserver(vtkCommand::LeftButtonPressEvent, click_callback); 

    interactor_style = vtkSmartPointer<vtkSimpInteractor>::New();
    interactor_style->set_model(model);
    interactor_style->set_view(view);
    interactor->SetInteractorStyle(interactor_style);
}

void DVIDPlaneController::start()
{
    view->start();
}

