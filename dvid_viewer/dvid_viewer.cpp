/*!
 * \file
 * The following program allows a user to view a label volume
 * in DVID.  Current implementation does not use tiling system
 * and is restricted to 500x500 slices.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

// main GUI model
#include "Model/Model.h"

// main GUI controller
#include "Gui/DVIDController.h"

#include <QApplication>
#include <string>
#include <iostream>
#include "OptionParser.h"
#include <libdvid/DVIDNode.h>

using namespace DVIDViewer;
using std::string;
using std::cout; using std::cerr; using std::endl;
using std::vector;

struct BuildOptions
{
    BuildOptions(int argc, char** argv) : x(0), y(0), z(0), x2(0), y2(0), z2(0)
    {
        OptionParser parser("Program that loads DVID volume for selected region");

        // dvid string arguments
        parser.add_option(dvid_servername, "dvid-server",
                "name of dvid server", false, true); 
        parser.add_option(uuid, "uuid",
                "dvid node uuid", false, true); 
        parser.add_option(labels_name, "label-name",
                "name of the label volume"); 

        // roi for image
        parser.add_option(x, "x", "x starting point"); 
        parser.add_option(y, "y", "y starting point"); 
        parser.add_option(z, "z", "z starting point"); 
        
        parser.add_option(x2, "x2", "x ending point"); 
        parser.add_option(y2, "y2", "y ending point"); 
        parser.add_option(z2, "z2", "z ending point"); 
        
        parser.add_option(roi, "roi", "roi"); 

        parser.parse_options(argc, argv);
    }

    // manadatory optionals
    string dvid_servername;
    string uuid;
    string labels_name; 
    string roi;

    int x, y, z, x2, y2, z2;
};


/*!
 * Entry point for program
 * \param argc number of arguments
 * \param argv array of char* arguments
 * \return status of program (0 for proper exit)
*/
int main(int argc, char** argv)
{
    BuildOptions options(argc, argv);
    // load QT application
    QApplication qapp(argc, argv);
   
    int x1,x2,y1,y2,z1,z2;

    // load grayscale info by default
    libdvid::DVIDServer server(options.dvid_servername);
    libdvid::DVIDNode dvid_node(server, options.uuid);

    Json::Value data;
    dvid_node.get_typeinfo("grayscale", data);
    x1 = data["Extended"]["MinPoint"][(unsigned int)(0)].asInt();
    y1 = data["Extended"]["MinPoint"][(unsigned int)(1)].asInt();
    z1 = data["Extended"]["MinPoint"][(unsigned int)(2)].asInt();
    x2 = data["Extended"]["MaxPoint"][(unsigned int)(0)].asInt();
    y2 = data["Extended"]["MaxPoint"][(unsigned int)(1)].asInt();
    z2 = data["Extended"]["MaxPoint"][(unsigned int)(2)].asInt();

    // load segmentation info if available
    try {
        dvid_node.get_typeinfo(options.labels_name, data);
        x1 = data["Extended"]["MinPoint"][(unsigned int)(0)].asInt();
        y1 = data["Extended"]["MinPoint"][(unsigned int)(1)].asInt();
        z1 = data["Extended"]["MinPoint"][(unsigned int)(2)].asInt();
        x2 = data["Extended"]["MaxPoint"][(unsigned int)(0)].asInt();
        y2 = data["Extended"]["MaxPoint"][(unsigned int)(1)].asInt();
        z2 = data["Extended"]["MaxPoint"][(unsigned int)(2)].asInt();
    } catch (...) {
    }

    // load roi if provided
    if (options.roi != "") {
        // load segmentation info if available
        try {
            dvid_node.get_typeinfo(options.roi, data);
            x1 = data["Extended"]["MinPoint"][(unsigned int)(0)].asInt();
            y1 = data["Extended"]["MinPoint"][(unsigned int)(1)].asInt();
            z1 = data["Extended"]["MinPoint"][(unsigned int)(2)].asInt();
            x2 = data["Extended"]["MaxPoint"][(unsigned int)(0)].asInt();
            y2 = data["Extended"]["MaxPoint"][(unsigned int)(1)].asInt();
            z2 = data["Extended"]["MaxPoint"][(unsigned int)(2)].asInt();
        } catch (...) {
        }
    }

    // load bbox if provided
    if ((options.x != options.x2) && (options.y != options.y2)
            && (options.z != options.z2)) {
        x1 = options.x; x2 = options.x2;
        y1 = options.y; y2 = options.y2;
        z1 = options.z; z2 = options.z2;
    } 
   
    Model* session = new Model(options.dvid_servername, options.uuid,
            options.labels_name, x1, y1, z1, x2, y2, z2); 

    // initialize controller with previous session or empty session  
    DVIDController controller(session, &qapp);

    // start gui
    qapp.exec();

    return 0;
}
