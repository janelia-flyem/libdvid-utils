#include "Model.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <libdvid/DVIDNode.h>

using namespace DVIDViewer;
using std::tr1::unordered_map;
using namespace boost::algorithm;
using namespace boost::filesystem;
using std::string;
using std::vector;
using std::ifstream; using std::ofstream;
using std::cout; using std::endl;

// to be called from command line
Model::Model(string dvid_servername, string uuid, string labels_name_,
        int x1, int y1, int z1, int x2, int y2, int z2) : server(dvid_servername),
    dvid_node(server, uuid), labels_name(labels_name_)
{
    // set all initial variables
    initialize();
    
    // setup volume info and starting location
    session_info.x = (x2-x1)/2 + x1;
    session_info.y = (y2-y1)/2 + y1; 
    session_info.curr_plane = (z2-z1)/2 + z1;

    session_info.lastx = INT_MAX;
    session_info.lasty = INT_MAX;
    session_info.lastplane = INT_MAX;

    session_info.width = 500;
    session_info.height = 500;
    session_info.minplane = z1;
    session_info.maxplane = z2;

    // load initial slices
    label_data = new unsigned int [session_info.width*session_info.height];
    set_plane(session_info.curr_plane);
}

int Model::min_plane()
{
    return session_info.minplane;
}

int Model::max_plane()
{
    return session_info.maxplane;
}

void Model::pan(int xshift, int yshift)
{
    session_info.x += xshift;
    session_info.y += yshift;
    set_reset_stack();
}

unsigned int Model::shape(unsigned int pos)
{
    if (pos == 0) {
        return session_info.width;
    } else if (pos == 1) {
        return session_info.height;
    } else if (pos == 2) {
        return 1;
    }
}

int Model::curr_xloc()
{
    return session_info.x;
}

int Model::curr_yloc()
{
    return session_info.y;
}

void Model::load_slices()
{
    if ((session_info.x == session_info.lastx) &&
       (session_info.y == session_info.lasty) &&
       (session_info.curr_plane == session_info.lastplane)) {
        return;
    }

    session_info.lastx = session_info.x;
    session_info.lasty = session_info.y;
    session_info.lastplane = session_info.curr_plane;

    int startx = session_info.x - session_info.width/2;
    int starty = session_info.y - session_info.height/2;

    libdvid::tuple start; start.push_back(startx); start.push_back(starty);
        start.push_back(session_info.curr_plane);
    libdvid::tuple sizes; sizes.push_back(session_info.width);
        sizes.push_back(session_info.height); sizes.push_back(1);
    libdvid::tuple channels; channels.push_back(0); channels.push_back(1); channels.push_back(2);

    olabels = labels;
    ograys = grays;
    
    dvid_node.get_volume_roi(labels_name, start, sizes, channels, labels);
    dvid_node.get_volume_roi("grayscale", start, sizes, channels, grays);

    Label_t* all_labels = labels->get_raw();
    int tsize = session_info.width * session_info.height;
    for (int i = 0; i < tsize; ++i) {
        label_data[i] = all_labels[i]; 
    }

}

unsigned char* Model::data()
{
    return grays->get_raw();
}

void Model::set_location(int x, int y, int z)
{
    session_info.x = x;
    session_info.y = y;
    session_info.curr_plane = z;
    set_plane(z);
}

unsigned int* Model::ldata()
{
    return label_data;
}

void Model::increment_plane()
{
    set_plane(active_plane+1);
}

void Model::initialize()
{
    saved_opacity = 4;
    curr_opacity = 4;
    label_data = 0;
    active_labels_changed = false;
    selected_id = 0;
    old_selected_id = 0;
    selected_id_changed = false;
    show_all = true;
    show_all_changed = false;
    active_plane = 0;
    active_plane_changed = false;
    opacity = 3;
    opacity_changed = false;
    reset_stack = false;
}

void Model::decrement_plane()
{
    set_plane(active_plane-1);
}


void Model::set_reset_stack()
{
    // pull data
    load_slices();

    reset_stack = true;
    update_all();
    reset_stack = false;
}

bool Model::get_reset_stack()
{
    return reset_stack;
}

void Model::set_opacity(unsigned int opacity_)
{   
    opacity = opacity_;
    curr_opacity = opacity_; 
    opacity_changed = true;
    update_all();
    opacity_changed = false;
}

void Model::set_plane(int plane)
{   
    active_plane = plane; 
    active_plane_changed = true;
    session_info.curr_plane = active_plane;
   
    set_reset_stack(); 
    active_plane_changed = false;
}

void Model::toggle_show_all()
{
    if (curr_opacity > 0) {
        saved_opacity = curr_opacity;
        set_opacity(0);
    } else {
        set_opacity(saved_opacity);
    }
}

bool Model::get_plane(int& plane_id)
{
    plane_id = active_plane;
    return active_plane_changed;
}

bool Model::get_opacity(unsigned int& opacity_)
{
    opacity_ = opacity;
    return opacity_changed;
}
bool Model::get_show_all(bool& show_all_)
{
    show_all_ = show_all;
    return show_all_changed;
}

bool Model::get_select_label(Label_t& select_curr, Label_t& select_old)
{
    select_curr = selected_id;
    select_old = old_selected_id;
    return selected_id_changed;
}

void Model::get_rgb(int color_id, unsigned char& r,
        unsigned char& g, unsigned char& b)
{
    unsigned int val = color_id % 18; 
    switch (val) {
        case 0:
            r = 0xff; g = b = 0; break;
        case 1:
            g = 0xff; r = b = 0; break;
        case 2:
            b = 0xff; r = g = 0; break;
        case 3:
            r = g = 0xff; b = 0; break;
        case 14:
            r = b = 0xff; g = 0; break;
        case 8:
            g = b = 0xff; r = 0; break;
        case 6:
            r = 0x7f; g = b = 0; break;
        case 16:
            g = 0x7f; r = b = 0; break;
        case 9:
            b = 0x7f; r = g = 0; break;
        case 5:
            r = g = 0x7f; b = 0; break;
        case 13:
            r = b = 0x7f; g = 0; break;
        case 11:
            g = b = 0x7f; r = 0; break;
        case 12:
            r = 0xff; g = b = 0x7f; break;
        case 10:
            g = 0xff; r = b = 0x7f; break;
        case 17:
            b = 0xff; r = g = 0x7f; break;
        case 15:
            r = g = 0xff; b = 0x7f; break;
        case 7:
            r = b = 0xff; g = 0x7f; break;
        case 4:
            g = b = 0xff; r = 0x7f; break;
    }
}

void Model::add_active_label(Label_t label)
{
    active_labels[label] = label % 18;
}

void Model::active_label(unsigned int x, unsigned int y, unsigned int z)
{
    Label_t current_label = labels->get_raw()[x+y*session_info.width];
    
    if (!current_label) {
        // ignore selection if off image or on boundary
        return;
    }
   
    if (active_labels.find(current_label) != active_labels.end()) {
        active_labels.erase(current_label);        
    } else {
        add_active_label(current_label); 
    }
    
    select_label(selected_id);

    active_labels_changed = true;
    update_all();
    active_labels_changed = false;
}



void Model::select_label(unsigned int x, unsigned int y, unsigned int z)
{
    Label_t current_label = labels->get_raw()[x+y*session_info.width];
    select_label(current_label);    
}

void Model::select_label(Label_t current_label)
{
    if (!current_label) {
        // ignore selection if off image or on boundary
        return;
    }
    if (!active_labels.empty() &&
            (active_labels.find(current_label) == active_labels.end())) {
        return;
    }
    
    old_selected_id = selected_id;
    if (current_label != selected_id) {
        selected_id = current_label;
    } else {
        selected_id = 0;
    }
    selected_id_changed = true;
    update_all();
    selected_id_changed = false;
}

bool Model::get_active_labels(unordered_map<Label_t, int>& active_labels_)
{
    active_labels_ = active_labels;
    return active_labels_changed;
}

void Model::reset_active_labels()
{
    active_labels.clear();
    active_labels_changed = true;
    show_all = true;
    show_all_changed = true;
    update_all();
    active_labels_changed = false;
    show_all_changed = false;
}


