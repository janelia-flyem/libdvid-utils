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
                "name of the label volume", false, true); 


        // roi for image
        parser.add_option(x, "x", "x starting point", false, true); 
        parser.add_option(y, "y", "y starting point", false, true); 
        parser.add_option(z, "z", "z starting point", false, true); 
        
        parser.add_option(x2, "x2", "x ending point", false, true); 
        parser.add_option(y2, "y2", "y ending point", false, true); 
        parser.add_option(z2, "z2", "z ending point", false, true); 

        parser.parse_options(argc, argv);
    }

    // manadatory optionals
    string dvid_servername;
    string uuid;
    string labels_name; 

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
    
    Model* session = new Model(options.dvid_servername, options.uuid,
            options.labels_name, options.x, options.y, options.z, options.x2,
            options.y2, options.z2);

    // initialize controller with previous session or empty session  
    DVIDController controller(session, &qapp);

    // start gui
    qapp.exec();

    return 0;
}
