/*!
 * Major model for in dvid viewer MVC.  
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef MODEL_H
#define MODEL_H

#include "Dispatcher.h"
#include <tr1/unordered_map>
#include <string>
#include <libdvid/DVIDNode.h>

namespace DVIDViewer {

typedef unsigned long long Label_t;

class Model : public Dispatcher {
  public:
    Model(std::string dvid_servername, std::string uuid, std::string labels_name_,
        int x1, int y1, int z1, int x2, int y2, int z2);
    
    /*!
     * For a given color id, an RGB value is derived.
     * \param color_id color id
     * \param r 8-bit red value
     * \param g 8-bit green value
     * \param b 8-bit blue value
    */
    void get_rgb(int color_id, unsigned char& r,
            unsigned char& g, unsigned char& b);
    
    /*!
     * Retrieve the selected active labels in the current session.  This
     * function is often used to probe the session state after observers
     * are updated.
     * \param active_labels_ map of labels to colors that are currently active
     * \return true if the active labels have changed for the current dispatch
    */
    bool get_active_labels(std::tr1::unordered_map<Label_t, int>& active_labels_);
    
    /*!
     * Retrieve which label was currently clicked.  This is not an active label
     * but typically indicates whether a label color should be toggled on or off.
     * \param select_curr label id selected
     * \param select_old last label id selected
     * \return true if the selected label changed for the current dispatch
    */
    bool get_select_label(Label_t& select_curr, Label_t& select_old);
    
    bool get_select_label_actual(Label_t& select_curr);

    /*!
     * Probes state on whether all bodies should be highlighted or not.
     * \param show_all_ true if all bodies label colors should be shown
     * \return true if this variable changed for the current dispatch
    */
    bool get_show_all(bool& show_all_);
    
    /*!
     * Probes state on whether the stack has been reset meening that
     * the stack label volume and RAG may have changed.
     * \return true if the stack was reset for the current dispatch
    */
    bool get_reset_stack();

    int curr_xloc();
    int curr_yloc();
    
    /*!
     * Retrieves the current plane being examined in the stack.
     * \param plane_id current plane
     * \return true if this plane was changed for the current dispatch
    */
    bool get_plane(int& plane_id);
    
    /*!
     * Retrieves the current opacity of color in the session.
     * \param opacity_ opacity value of the color
     * \return true if this value changed for the current dispatch
    */
    bool get_opacity(unsigned int& opacity_);
   
    /*!
     * Mechanism for resetting the stack which will cause will
     * empty all the active labels, reset the zoom, reset the undo
     * queue, rest the colors, and force observers to update the
     * label volume and RAG.
    */ 
    void set_reset_stack();
    
    /*!
     * Increase plane value (z) by 1.
    */
    void increment_plane();
    
    /*!
     * Decrease plane value (z) by 1.
    */
    void decrement_plane();
    
    /*!
     * Toggle the coloring of all of the labels.
    */
    void toggle_show_all();
    
    /*!
     * Jump to a specified plane.
     * \param plane id for a given plane in the stack
    */
    void set_plane(int plane);
   
    /*!
     * Set a certain opacity for color.  0 means no color
     * and 10 means opaque.
     * \param opacity integer value for opacity
    */
    void set_opacity(unsigned int opacity_);

    int max_plane();
    int min_plane();

    /*!
     * Selects a label (for toggling color typically) for the given
     * x, y, and z location.
     * \param x integer for x location in stack
     * \param y integer for y location in stack
     * \param z integer for z location in stack
    */
    void select_label(unsigned int x, unsigned int y, unsigned z);
    void select_label_actual(unsigned int x, unsigned int y, unsigned int z);
    
    /*!
     * Selects a label to be added or removed from the active
     * label list for the given x, y, and z location.
     * \param x integer for x location in stack
     * \param y integer for y location in stack
     * \param z integer for z location in stack
    */
    void active_label(unsigned int x, unsigned int y, unsigned z);
    
    /*!
     * Erase all of the active labels.
    */
    void reset_active_labels();
   
    unsigned char* data();
    unsigned int* ldata();

    void pan(int xshift, int yshift);

    unsigned int shape(unsigned int pos);
    void set_location(int x, int y, int z);
    
    void set_incr_factor(int incr_factor_);
    void set_pan_factor(int pan_factor_);

  private:
    /*!
     * Keep track of certain labels by adding to an active list.
     * \param label volume label
    */
    void add_active_label(Label_t label);

  public:
    /*!
     * Helper function to select a given label.
     * \param current_label label selected
    */ 
    void select_label(Label_t current_label);
   
    void select_label_actual(Label_t current_label);
  private:
    struct SessionInfo {
        int x, y;
        int curr_plane;
        int width, height;
        int minplane;
        int maxplane;
        int lastx, lasty, lastplane;
    };

    SessionInfo session_info;
    
    libdvid::DVIDLabelPtr labels;
    libdvid::DVIDGrayPtr grays;
    libdvid::DVIDLabelPtr olabels;
    unsigned int * label_data;
    unsigned int * olabel_data;
    libdvid::DVIDGrayPtr ograys;

    void load_slices(); 
    
    /*!
     * Sets all values to their default.  Called by the constructors.
    */
    void initialize();
    
    //! contains a set of active labels for examination
    std::tr1::unordered_map<Label_t, int> active_labels;
   
    //! true if active labels have changed
    bool active_labels_changed;

    //! current label id selected (doesn't have to be an active label)
    Label_t selected_id;
    
    Label_t selected_id_actual;

    //! previous label id selected
    Label_t old_selected_id;

    //! true if selected id changed
    bool selected_id_changed;
    
    //! true if selected id changed
    bool selected_id_changed_actual;

    //! true if all labels should be colored
    bool show_all;

    //! true if show all value changed
    bool show_all_changed;

    //! current active plane
    int active_plane;

    //! current opacity of color (0 transparent, 10 opaque)
    unsigned int opacity;

    //! true if active plane changed
    bool active_plane_changed;

    //! true if opactiy changed
    bool opacity_changed;

    //! true if the stack was reset
    bool reset_stack;

    //! current x and y locations in the stack
    unsigned int x_zoom, y_zoom;

    //! true if zoom location changed
    bool zoom_loc;

    //! factor of zoom (>1 zoom in)
    double zoom_factor;

    unsigned int saved_opacity;
    unsigned int curr_opacity;

    int pan_factor;
    int incr_factor;

    libdvid::DVIDServer server;
    libdvid::DVIDNode dvid_node;
    std::string labels_name;
};

}


#endif
