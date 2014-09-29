/*!
 * Functionality for the main window QT widget controller.
 * It also helps coordinate functioanlity with other GUI
 * controllers such as the plane and body controller.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef DVIDCONTROLLER_H
#define DVIDCONTROLLER_H

#include "../Model/ModelObserver.h"
#include <QWidget>

class QApplication;

namespace DVIDViewer {

class Model;
class DVIDUi;
class DVIDPlaneController;

/*!
 * Controller of type QObject that handles events emitted from
 * the qt main window view.  The controller helps call the appropriate
 * functions to update the stack session model that dispatches
 * events for all of its listeners.  Technically, this class is
 * also an observer, but it currently does not perform any actions
 * when the stack session state changes.
*/
class DVIDController : public QObject, public ModelObserver {
    Q_OBJECT
  
  public:
    /*!
     * Constructor that creates the main qt window and loads the
     * stack session, if provided, after a small time delay.  The
     * constructor also connects Qt signals to the proper functions.
    */
    DVIDController(Model* model_, QApplication* qapp_);
   
    ~DVIDController();

    /*!
     * No actions are performed when the stack session dispatches
     * an event.
    */
    void update();

  private:
    /*!
     * Internal function called when a new session is created or
     * when the controller is deleted.  It will destroy all created
     * controllers and the session.
    */  
    void clear_session();
    
    /*! Internal function called when in the training mode to update
     * the progress through the edges examined.
    */
    void update_progress();

    Model* model;

    //! Qt application object
    QApplication* qapp;

    //! main Qt view
    DVIDUi* main_ui;

    //! controls the widget for planar views of the stack
    DVIDPlaneController* plane_controller;

  private slots:
    /*!
     * Handles the intial timer event if the constructor is supplied
     * with a stack session.  Otherwise, this is an internal function
     * that initializes the plane controller for the new stack session
     * and connects some of its functionality to Qt signals.
    */
    void load_views();
    
    /*!
     * Handles the shortcuts click event.  It provides information
     * on the current shortcuts used in the program.  TODO: allow 
     * the user to change these shortcuts.
    */
    void show_shortcuts();

    void quit_program();
    
    /*!
     * Handles the about click event.  It provides information about
     * the stack viewer program.
    */
    void show_about();
    
    /*!
     * Handles the help click event.  It provides information about
     * how to use the stack viewer program.
    */
    void show_help();
   
    /*!
     * Handles toggle of body id search.
    */
    void location_search();
};

}

#endif
