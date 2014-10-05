#include "DVIDPlaneView.h"
#include "../Model/Model.h"
#include "DVIDPlaneController.h"

#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkRenderWindow.h>
#include <vtkInteractorStyle.h>
#include <vtkPointData.h>
#include "QVTKWidget.h"
#include <QtGui/QWidget>
#include <QVBoxLayout>

using namespace DVIDViewer;
using std::tr1::unordered_map;
using std::vector;

DVIDPlaneView::DVIDPlaneView(Model* model_, 
        DVIDPlaneController* controller_, QWidget* widget_parent_) : 
        model(model_), controller(controller_),
        widget_parent(widget_parent_), renderWindowInteractor(0)
{
    model->attach_observer(this);
} 

DVIDPlaneView::~DVIDPlaneView()
{
    model->detach_observer(this);
    if (qt_widget) {
        if (layout) {
            QLayoutItem * item;
            if ((item = layout->takeAt(0)) != 0) {
                layout->removeItem(item);
            }
            layout->removeWidget(qt_widget);
            delete layout;
        }
        delete qt_widget;
    }
}
    
// call update and create controller -- rag, gray, and labels must exist
void DVIDPlaneView::initialize()
{
    // load gray volume into vtk array
    grayarray = vtkSmartPointer<vtkUnsignedCharArray>::New();
    grayarray->SetArray((unsigned char *)(model->data()), model->shape(0) * 
            model->shape(1) * model->shape(2), 1);
    
    // load array into gray vtk image volume
    grayvtk = vtkSmartPointer<vtkImageData>::New();
    grayvtk->GetPointData()->SetScalars(grayarray); 

    // set grayscale properties
    grayvtk->SetDimensions(model->shape(0), model->shape(1),
            model->shape(2));
    grayvtk->SetScalarType(VTK_UNSIGNED_CHAR);
    grayvtk->SetSpacing(1, 1, 1); // hack for now; TODO: set res from stack?
    grayvtk->SetOrigin(0.0, 0.0, 0.0);

    graylookup = vtkSmartPointer<vtkLookupTable>::New();
    graylookup->SetNumberOfTableValues(256);
    graylookup->SetRange(0.0, 256);
    graylookup->SetHueRange(0.0,1.0);
    graylookup->SetValueRange(0.0,1.0);
    graylookup->Build();
    for (int i = 0; i < 256; ++i) {
        graylookup->SetTableValue(i, i/255.0, i/255.0, i/255.0, 1);
    }
    gray_mapped = vtkSmartPointer<vtkImageMapToColors>::New();
    gray_mapped->SetInput(grayvtk);
    gray_mapped->SetLookupTable(graylookup);
    gray_mapped->Update();

    // 32 bit array takes some time to load -- could convert the 32 bit
    // image to a 8 bit uchar for color
    labelarray = vtkSmartPointer<vtkUnsignedIntArray>::New();
    labelarray->SetArray(model->ldata(), model->shape(0) * model->shape(1) *
            model->shape(2), 1);

    // set label options
    labelvtk = vtkSmartPointer<vtkImageData>::New();
    labelvtk->GetPointData()->SetScalars(labelarray);
    labelvtk->SetDimensions(model->shape(0), model->shape(1),
            model->shape(2));
    labelvtk->SetScalarType(VTK_UNSIGNED_INT);
    labelvtk->SetSpacing(1, 1, 1);
    labelvtk->SetOrigin(0.0, 0.0, 0.0);

    // set lookup table
    label_lookup = vtkSmartPointer<vtkLookupTable>::New();
    label_lookup->SetNumberOfTableValues(model->color_table_size());
    label_lookup->SetRange( 0.0, model->color_table_size()-1); 
    label_lookup->SetHueRange( 0.0, 1.0 );
    label_lookup->SetValueRange( 0.0, 1.0 );
    label_lookup->Build();

    load_colors();
    
    // map colors for each label
    labelvtk_mapped = vtkSmartPointer<vtkImageMapToColors>::New();
    labelvtk_mapped->SetInput(labelvtk);
    labelvtk_mapped->SetLookupTable(label_lookup);
    labelvtk_mapped->Update();


    // blend both images
    vtkblend = vtkSmartPointer<vtkImageBlend>::New();
    vtkblend->AddInputConnection(gray_mapped->GetOutputPort());
    vtkblend->AddInputConnection(labelvtk_mapped->GetOutputPort());
    vtkblend->SetOpacity(0, 1);
    vtkblend->SetOpacity(1, 0.3);
    vtkblend->Update();
    
    // flip along y
    vtkblend_flipped = vtkSmartPointer<vtkImageFlip>::New();
    vtkblend_flipped->SetInputConnection(vtkblend->GetOutputPort());
    vtkblend_flipped->SetFilteredAxis(1);
    vtkblend_flipped->Update();


    // create 2D view
    viewer = vtkSmartPointer<vtkImageViewer2>::New();
    viewer->SetColorLevel(127.5);
    viewer->SetColorWindow(255);
    viewer->SetInputConnection(vtkblend_flipped->GetOutputPort());

    //qt widgets
   
    if (widget_parent) {
        layout = new QVBoxLayout;
        qt_widget = new QVTKWidget;
        layout->addWidget(qt_widget);
        widget_parent->setLayout(layout);
        //qt_widget = new QVTKWidget(widget_parent);
        //planeView->setGeometry(QRect(0, 0, 671, 571));
    } else {
        qt_widget = new QVTKWidget;
    }

    //qt_widget->setObjectName(QString::fromUtf8("planeView"));
    //qt_widget->showMaximized();
    qt_widget->SetRenderWindow(viewer->GetRenderWindow());
    renderWindowInteractor = qt_widget->GetInteractor();

    viewer->SetupInteractor(renderWindowInteractor);
    viewer->Render();
    vtkRenderer * renderer = viewer->GetRenderer();  
    vtkCamera *camera = renderer->GetActiveCamera();
    initial_zoom = camera->GetParallelScale();
}

void DVIDPlaneView::load_colors()
{
    for (int i = 0; i < model->color_table_size(); ++i) {    
        unsigned char r, g, b;
        model->get_rgb(i, r, g, b);
        label_lookup->SetTableValue(i, r/255.0,
                g/255.0, b/255.0, 1);
    }
}

void DVIDPlaneView::start()
{
    qt_widget->show();
}

void DVIDPlaneView::update()
{
    bool show_all;
    unsigned int plane_id;
    vector<Label_t> select_id;
    vector<Label_t> select_id_old;
    Label_t ignore_label = 0;
    double rgba[4];

    if (model->get_reset_stack()) {
        // set grays 
        grayarray = vtkSmartPointer<vtkUnsignedCharArray>::New();
        grayarray->SetArray((unsigned char *)(model->data()), model->shape(0) * 
                model->shape(1) * model->shape(2), 1);
        grayvtk->GetPointData()->SetScalars(grayarray); 

        // set labels 
        labelarray = vtkSmartPointer<vtkUnsignedIntArray>::New();
        labelarray->SetArray(model->ldata(), model->shape(0) * model->shape(1) *
                model->shape(2), 1);
        labelvtk->GetPointData()->SetScalars(labelarray);
        
        labelvtk->Modified();
        grayvtk->Modified();
    }

    // read mappings and leftovers and udpate
    unordered_map<Label_t, Label_t> remap_labels;
    vector<Label_t> reset_labels;
    if (model->get_mapping_changed(remap_labels, reset_labels)) {
        for (unordered_map<Label_t, Label_t>::iterator iter = remap_labels.begin();
                iter != remap_labels.end(); ++iter) {
            unsigned char r, g, b;
            label_lookup->GetTableValue(iter->second, rgba);
            model->get_rgb(iter->second, r, g, b);
            rgba[0] = r/255.0;
            rgba[1] = g/255.0;
            rgba[2] = b/255.0;
            label_lookup->SetTableValue(iter->first, rgba);
        }
        for (int i = 0; i < reset_labels.size(); ++i) {
            unsigned char r, g, b;
            label_lookup->GetTableValue(reset_labels[i], rgba);
            model->get_rgb(reset_labels[i], r, g, b);
            rgba[0] = r/255.0;
            rgba[1] = g/255.0;
            rgba[2] = b/255.0;
            label_lookup->SetTableValue(reset_labels[i], rgba);
        }
        label_lookup->Modified();
    }

    // toggle color for clicked body label
    // set all select alls sps belong to body as well
    if (model->get_select_label(select_id, select_id_old)) {
        for (int i = 0; i < select_id_old.size(); ++i) {
            label_lookup->GetTableValue(select_id_old[i], rgba);
            rgba[3] = 1.0;
            label_lookup->SetTableValue(select_id_old[i], rgba);
            label_lookup->Modified();
        } 
    }
   
    // set the current color opacity
    unsigned int curr_opacity = 0;
    if (model->get_opacity(curr_opacity)) {
        vtkblend->SetOpacity(1, curr_opacity / 10.0);
    }

    bool reverse_label = false;
    if (model->get_reverse_select(reverse_label)) {
        double opacity_val = 1.0;
        if (reverse_label) {
            opacity_val = 0.0;
        }
        for (int i = 0; i < label_lookup->GetNumberOfTableValues(); ++i) {
            label_lookup->GetTableValue(i, rgba);
            rgba[3] = opacity_val;
            label_lookup->SetTableValue(i, rgba);
        }
    }

    for (int i = 0; i < select_id_old.size(); ++i) {
        double opacity_val = 1.0;
        if (reverse_label) {
            opacity_val = 0.0;
        }

        label_lookup->GetTableValue(select_id_old[i], rgba);
        rgba[3] = opacity_val;
        label_lookup->SetTableValue(select_id_old[i], rgba);
        label_lookup->Modified();
    }

    for (int i = 0; i < select_id.size(); ++i) {
        double opacity_val = 0.0;
        if (reverse_label) {
            opacity_val = 1.0;
        }
        
        label_lookup->GetTableValue(select_id[i], rgba);
        rgba[3] = opacity_val;
        label_lookup->SetTableValue(select_id[i], rgba);
        label_lookup->Modified();
    }

    viewer->Render();
}
