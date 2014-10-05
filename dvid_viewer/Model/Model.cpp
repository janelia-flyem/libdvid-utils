#include "Model.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <libdvid/DVIDNode.h>
#include <sstream>

using std::stringstream;
using namespace DVIDViewer;
using std::tr1::unordered_map;
using namespace boost::algorithm;
using namespace boost::filesystem;
using std::string;
using std::vector;
using std::ifstream; using std::ofstream;
using std::cout; using std::endl;
using std::tr1::unordered_set;
using std::deque;

// implement merge queue functionality
bool MergeQueue::add_decision(Decision& decision)
{
    queue.push_back(decision);

    // load reverse maps
    if (label_sets.find(decision.master) == label_sets.end()) {
        label_sets[decision.master].insert(decision.master);
    }
    if (label_sets.find(decision.slave) == label_sets.end()) {
        label_sets[decision.master].insert(decision.slave);
    } else {
        label_sets[decision.master].insert(label_sets[decision.slave].begin(), label_sets[decision.slave].end());
        // don't erase becaue this is used in undo
        //label_sets.erase(decision.slave);
    }

    // load forward map
    for (unordered_set<Label_t>::iterator iter = label_sets[decision.master].begin();
            iter != label_sets[decision.master].end(); ++iter) {
        label_mapping[*iter]=  decision.master;
    }

    // save if past queue limit
    bool saved_decision = false;   
    if (queue.size() > queue_limit) {
        save_decision_to_dvid();
        saved_decision = true;
    }

    return saved_decision;
}
    
bool MergeQueue::undo_decision(Decision& decision)
{
    if (queue.empty()) {
        return false;
    }
    decision = queue.back();
    queue.pop_back();

    if (label_sets.find(decision.slave) != label_sets.end()) {
        for (unordered_set<Label_t>::iterator iter = label_sets[decision.slave].begin();
                iter != label_sets[decision.master].end(); ++iter) {
            label_mapping[*iter] =  decision.slave;
            label_sets[decision.master].erase(*iter);
        } 
    } else {
        label_sets[decision.master].erase(decision.slave);
        label_mapping[decision.slave] = decision.slave;
    }

    return true;
}


void MergeQueue::save()
{
    while (!queue.empty()) {
        save_decision_to_dvid();
    }
    label_mapping.clear();
    label_sets.clear();
}


void MergeQueue::get_mappings(std::tr1::unordered_map<Label_t, Label_t>& 
            label_mapping_, std::vector<Label_t>& recently_retired_)
{
    label_mapping_ = label_mapping;
    recently_retired_ = recently_retired;
}

void MergeQueue::get_reverse_map(Label_t label, std::tr1::unordered_set<Label_t>& mapped_vals)
{
    if (label_sets.find(label) == label_sets.end()) {
        unordered_set<Label_t> temp_set;
        temp_set.insert(label);
        mapped_vals = temp_set;
    } else {
        mapped_vals = label_sets[label];
    }
}
    
void MergeQueue::save_decision_to_dvid()
{
    Decision decision = queue.front();
    queue.pop_front();
    recently_retired.push_back(decision.slave);
    label_mapping.erase(decision.slave);
    label_sets.erase(decision.slave);
    label_sets[decision.master].erase(decision.slave);

    // ?! load into DVID
    // dvid_node.merge_labels(label_map, 
    cout << "Mapping " << decision.slave << " to " << decision.master 
        << " in DVID" << endl;
}

void MergeQueue::clear_retired()
{
    recently_retired.clear();
}

Label_t MergeQueue::get_label(Label_t label)
{
    if (label_mapping.find(label) != label_mapping.end()) {
        return label_mapping[label];
    }
    return label;
}



// to be called from command line
Model::Model(string dvid_servername, string uuid, string labels_name_,
        int x1, int y1, int z1, int x2, int y2, int z2) : server(dvid_servername),
    dvid_node(server, uuid), labels_name(labels_name_),
    merge_queue(5, dvid_node, labels_name)
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

    int tsize = session_info.width * session_info.height;
    for (int i = 0; i < tsize; ++i) {
        label_data[i] = 0; 
    }

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
    session_info.x += (xshift * pan_factor);
    session_info.y += (yshift * pan_factor);
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
   
    dvid_node.get_volume_roi("grayscale", start, sizes, channels, grays, false);
    if (labels_name != "") { 
        dvid_node.get_volume_roi(labels_name, start, sizes, channels, labels, false);
        Label_t* all_labels = labels->get_raw();
        int tsize = session_info.width * session_info.height;
        for (int i = 0; i < tsize; ++i) {
            label_data[i] = all_labels[i] & 0xfffff; 
        }
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
    set_plane(active_plane+incr_factor);
}

void Model::set_incr_factor(int incr_factor_)
{
    incr_factor = incr_factor_;
}

void Model::set_pan_factor(int pan_factor_)
{
    pan_factor = pan_factor_;
}

void Model::initialize()
{
    pan_factor = 250;
    incr_factor = 1;
    saved_opacity = 4;
    curr_opacity = 4;
    label_data = 0;
    selected_id = 0;
    selected_id_actual = 0;
    old_selected_id = 0;
    old_selected_id_actual = 0;
    selected_id_changed_actual = false;
    show_all = true;
    show_all_changed = false;
    active_plane = 0;
    active_plane_changed = false;
    opacity = 3;
    opacity_changed = false;
    reset_stack = false;
    reverse_select = false;
    reverse_select_changed = false;


    color_table.push_back(0);
    int r, g, b;
    for (int i = 1; i < 2000000; ++i) {    
        r = rand()%255;
        g = rand()%255;
        b = rand()%255;
        int temp1 = std::min(r,g);
        temp1 = std::min(temp1,b);
        int temp2 = std::max(r,g);
        temp2 = std::max(temp2,b);
        int diff = 100 - (temp2 - temp1);
        if (diff > 0) {
            int dec = std::min(diff, temp1);
            diff -= dec;
            if ((r < b) && (r < g)) {
                r -= dec; 
            } else if ((b < r) && (b < g)) {
                b -= dec;
            } else {
                g -= dec;
            }
        }
        if (diff > 0) {
            if ((r > b) && (r > g)) {
                r += diff; 
            } else if ((b > r) && (b > g)) {
                b += diff;
            } else {
                g += diff;
            }
        }
        color_table.push_back(r | g << 8 | b << 16); 
    }
}

int Model::color_table_size()
{
    return color_table.size();
}

void Model::set_reverse_select()
{
    reverse_select = !reverse_select;
    reverse_select_changed = true;
    update_all();
    reverse_select_changed = false;
}

bool Model::get_reverse_select(bool& reverse_select_)
{
    reverse_select_ = reverse_select;
    return reverse_select_changed;
}

void Model::decrement_plane()
{
    set_plane(active_plane-incr_factor);
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

bool Model::get_select_label_actual(Label_t& select_curr)
{
    select_curr = selected_id_actual;
    return selected_id_changed_actual;
}

bool Model::get_select_label(vector<Label_t>& select_curr, vector<Label_t>& select_old)
{
    unordered_set<Label_t> selected_ids;
    merge_queue.get_reverse_map(selected_id_actual, selected_ids);
    unordered_set<Label_t> old_selected_ids;
    merge_queue.get_reverse_map(old_selected_id_actual, old_selected_ids);

    for (unordered_set<Label_t>::iterator iter = selected_ids.begin();
            iter != selected_ids.end(); ++iter) {
        select_curr.push_back((*iter) & 0xfffff);
    }
    for (unordered_set<Label_t>::iterator iter = old_selected_ids.begin();
            iter != old_selected_ids.end(); ++iter) {
        select_old.push_back((*iter) & 0xfffff);
    }

    return selected_id_changed;
}

void Model::get_rgb(Label_t color_id, unsigned char& r,
        unsigned char& g, unsigned char& b)
{
    int val = color_table[color_id];
    r = (unsigned char)(val & 0xff);
    g = (unsigned char)((val >> 8) & 0xff);
    b = (unsigned char)((val >> 16) & 0xff);
}

void Model::add_active_label(Label_t label)
{
    active_labels[label] = label % 18;
}

void Model::active_label(unsigned int x, unsigned int y, unsigned int z)
{
    //Label_t current_label = labels->get_raw()[x+y*session_info.width];
    Label_t current_label = label_data[x+y*session_info.width];
    
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
    //Label_t current_label = labels->get_raw()[x+y*session_info.width];
    Label_t current_label = label_data[x+y*session_info.width];
    select_label_actual(x,y,z);    
    select_label(current_label);    
}

void Model::merge_label(unsigned int x, unsigned int y, unsigned int z)
{
    if (selected_id_actual != 0) {
        Label_t current_merge_label = label_data[x+y*session_info.width];
        if (current_merge_label == 0) {
            return;
        }
        
        Label_t master = merge_queue.get_label(selected_id_actual);
        Label_t slave = merge_queue.get_label(current_merge_label);
  
        if (master == slave) {
            return;
        }

        Decision decision;
        decision.master = master;
        decision.slave = slave;
        decision.x = session_info.x + x - session_info.width/2;
        decision.y = session_info.y + y - session_info.height/2;
        decision.z = session_info.curr_plane + z;

        bool saved_decision = merge_queue.add_decision(decision);
        stringstream sstr;
        sstr << "Merge: " << slave << " to " << master;

        // update color maps and status message 
        status_changed = true;
        status_message = sstr.str();
        mapping_changed = true;
        status_type = ACTION;

        if (saved_decision) {
            set_reset_stack();
        } else {
            update_all();
        }
        mapping_changed = false;
        merge_queue.clear_retired();
        status_changed = false;
    }
}

void Model::save_to_dvid()
{
    merge_queue.save();

    status_changed = true;
    status_message = "Saved Data";
    status_type = ACTION;
    mapping_changed = true;
    set_reset_stack();
    mapping_changed = false;
    merge_queue.clear_retired();
    status_changed = false; 
}

void Model::undo()
{
    Decision decision;
    if (merge_queue.undo_decision(decision)) {
        stringstream sstr;
        sstr << "Undo merge: " << decision.slave << " to " << decision.master;

        // update color maps and status message 
        status_changed = true;
        status_message = sstr.str();
        status_type = UNDOACTION;
        mapping_changed = true;

        set_location(decision.x, decision.y, decision.z);

        mapping_changed = false;
        status_changed = false;
        merge_queue.clear_retired();

        selected_id_actual = decision.slave;
        selected_id = decision.slave;
        select_label_actual(decision.master);    
        select_label(decision.master & 0xfffff);    
    } else {
        status_changed = true;
        status_message = "Undo queue empty";
        status_type = WARNING;
        update_all();
        status_changed = false;
    }
}

// need to return body to labels reverse map
bool Model::get_mapping_changed(unordered_map<Label_t, Label_t>&
        label_mapping, vector<Label_t>& recently_retired)
{
    unordered_map<Label_t, Label_t> label_mapping_pre;
    vector<Label_t> recently_retired_pre;
    merge_queue.get_mappings(label_mapping_pre, recently_retired_pre);

    // converts this to pruned session labels
    for (unordered_map<Label_t, Label_t>::iterator iter = label_mapping_pre.begin();
        iter != label_mapping_pre.end(); ++iter) {
        label_mapping[iter->first & 0xfffff] = iter->second & 0xfffff;
    }
    for (int i = 0; i < recently_retired_pre.size(); ++i) {
        recently_retired.push_back(recently_retired_pre[i] & 0xfffff);
    }

    return mapping_changed;
}

bool Model::get_status_message(string& status_message_, StatusEnum& type)
{
    status_message_ = status_message;
    type = status_type;
    return status_changed;
}

void Model::select_label_actual(unsigned int x, unsigned int y, unsigned int z)
{
    if (labels_name == "") {
        return;
    }
    Label_t current_label = labels->get_raw()[x+y*session_info.width];
    select_label_actual(merge_queue.get_label(current_label));    
}

void Model::select_label_actual(Label_t current_label)
{
    if (!current_label) {
        // ignore selection if off image or on boundary
        return;
    }
    old_selected_id_actual = selected_id_actual;
    if (current_label != selected_id_actual) {
        selected_id_actual = current_label;
    } else {
        selected_id_actual = 0;
    }
    selected_id_changed_actual = true;
    update_all();
    selected_id_changed_actual = false;
}

void Model::select_label(Label_t current_label)
{
    if (!current_label) {
        // ignore selection if off image or on boundary
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


