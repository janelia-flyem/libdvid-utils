/*!
 * Functionality for allowing views and controllers to
 * receive updates from the stack when changes are made.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef MODELOBSERVER_H
#define MODELOBSERVER_H

namespace DVIDViewer {

/*!
 * Interface base class for observing Model objects.
*/
class ModelObserver {
  public:
    /*!
     * Call observer update functionality when
     * the stack changes.
    */
    virtual void update() = 0;

  protected:
    /*!
     * Prevent destruction of observer through
     * the base class pointer.
    */
    virtual ~ModelObserver() {}
};

}

#endif
