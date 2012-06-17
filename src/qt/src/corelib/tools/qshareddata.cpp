/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qshareddata.h>

QT_BEGIN_NAMESPACE

/*! 
    \class QSharedData
    \brief The QSharedData class is a base class for shared data objects.
    \reentrant

    QSharedData is designed to be used with QSharedDataPointer or
    QExplicitlySharedDataPointer to implement custom \l{implicitly
    shared} or explicitly shared classes. QSharedData provides
    \l{thread-safe} reference counting.

    See QSharedDataPointer and QExplicitlySharedDataPointer for details.
*/

/*! \fn QSharedData::QSharedData()
    Constructs a QSharedData object with a reference count of 0.
*/

/*! \fn QSharedData::QSharedData(const QSharedData& other)
    Constructs a QSharedData object with reference count 0.
    \a other is ignored.
*/

/*! 
    \class QSharedDataPointer
    \brief The QSharedDataPointer class represents a pointer to an implicitly shared object.
    \since 4.0
    \reentrant

    QSharedDataPointer\<T\> makes writing your own \l {implicitly
    shared} classes easy. QSharedDataPointer implements \l {thread-safe}
    reference counting, ensuring that adding QSharedDataPointers to your
    \l {reentrant} classes won't make them non-reentrant.

    \l {Implicit sharing} is used by many Qt classes to combine the
    speed and memory efficiency of pointers with the ease of use of
    classes. See the \l{Shared Classes} page for more information.

    \target Employee example
    Suppose you want to make an \c Employee class implicitly shared. The
    procedure is:

    \list

    \o Define the class \c Employee to have a single data member of
     type \c {QSharedDataPointer<EmployeeData>}.

    \o Define the \c EmployeeData class derived from \l QSharedData to
     contain all the data members you would normally have put in the
     \c Employee class.

    \endlist

    To show this in practice, we review the source code for the
    implicitly shared \c Employee class. In the header file we define the
    two classes \c Employee and \c EmployeeData.

    \snippet doc/src/snippets/sharedemployee/employee.h 0

    In class \c Employee, note the single data member, a \e {d pointer}
    of type \c {QSharedDataPointer<EmployeeData>}. All accesses of
    employee data must go through the \e {d pointer's} \c
    {operator->()}.  For write accesses, \c {operator->()} will
    automatically call detach(), which creates a copy of the shared data
    object if the shared data object's reference count is greater than
    1. This ensures that writes to one \c Employee object don't affect
    any other \c Employee objects that share the same \c EmployeeData
    object.

    Class \c EmployeeData inherits QSharedData, which provides the
    \e{behind the scenes} reference counter. \c EmployeeData has a default
    constructor, a copy constructor, and a destructor. Normally, trivial
    implementations of these are all that is needed in the \e {data}
    class for an implicitly shared class.

    Implementing the two constructors for class \c Employee is also
    straightforward. Both create a new instance of \c EmployeeData
    and assign it to the \e{d pointer} .

    \snippet doc/src/snippets/sharedemployee/employee.h 1
    \codeline
    \snippet doc/src/snippets/sharedemployee/employee.h 2

    Note that class \c Employee also has a trivial copy constructor
    defined, which is not strictly required in this case.

    \snippet doc/src/snippets/sharedemployee/employee.h 7

    The copy constructor is not strictly required here, because class \c
    EmployeeData is included in the same file as class \c Employee
    (\c{employee.h}). However, including the private subclass of
    QSharedData in the same file as the public class containing the
    QSharedDataPointer is not typical. Normally, the idea is to hide the
    private subclass of QSharedData from the user by putting it in a
    separate file which would not be included in the public file. In
    this case, we would normally put class \c EmployeeData in a separate
    file, which would \e{not} be included in \c{employee.h}. Instead, we
    would just predeclare the private subclass \c EmployeeData in \c
    {employee.h} this way:

    \code
    class EmployeeData;
    \endcode

    If we had done it that way here, the copy constructor shown would be
    required. Since the copy constructor is trivial, you might as well
    just always include it.

    Behind the scenes, QSharedDataPointer automatically increments the
    reference count whenever an \c Employee object is copied, assigned,
    or passed as a parameter. It decrements the reference count whenever
    an \c Employee object is deleted or goes out of scope.  The shared
    \c EmployeeData object is deleted automatically if and when the
    reference count reaches 0.

    In a non-const member function of \c Employee, whenever the \e {d
    pointer} is dereferenced, QSharedDataPointer automatically calls
    detach() to ensure that the function operates on its own copy of the
    data.

    \snippet doc/src/snippets/sharedemployee/employee.h 3
    \codeline
    \snippet doc/src/snippets/sharedemployee/employee.h 4

    Note that if detach() is called more than once in a member function
    due to multiple dereferences of the \e {d pointer}, detach() will
    only create a copy of the shared data the first time it is called,
    if at all, because on the second and subsequent calls of detach(),
    the reference count will be 1 again.

    But note that in the second \c Employee constructor, which takes an
    employee ID and a name, both setId() and setName() are called, but
    they don't cause \e{copy on write}, because the reference count for
    the newly constructed \c EmployeeData object has just been set to 1.

    In \c Employee's \e const member functions, dereferencing the \e {d
    pointer} does \e not cause detach() to be called.

    \snippet doc/src/snippets/sharedemployee/employee.h 5
    \codeline
    \snippet doc/src/snippets/sharedemployee/employee.h 6

    Notice that there is no need to implement a copy constructor or an
    assignment operator for the \c Employee class, because the copy
    constructor and assignment operator provided by the C++ compiler
    will do the \e{member by member} shallow copy required. The only
    member to copy is the \e {d pointer}, which is a QSharedDataPointer,
    whose \c {operator=()} just increments the reference count of the
    shared \c EmployeeData object.

    \target Implicit vs Explicit Sharing
    \section1 Implicit vs Explicit Sharing

    Implicit sharing might not be right for the \c Employee class.
    Consider a simple example that creates two instances of the
    implicitly shared \c Employee class.

    \snippet doc/src/snippets/sharedemployee/main.cpp 0

    After the second employee e2 is created and e1 is assigned to it,
    both \c e1 and \c e2 refer to Albrecht Durer, employee 1001. Both \c
    Employee objects point to the same instance of \c EmployeeData,
    which has reference count 2. Then \c {e1.setName("Hans Holbein")} is
    called to change the employee name, but because the reference count
    is greater than 1, a \e{copy on write} is performed before the name
    is changed. Now \c e1 and \c e2 point to different \c EmployeeData
    objects. They have different names, but both have ID 1001, which is
    probably not what you want. You can, of course, just continue with
    \c {e1.setId(1002)}, if you really mean to create a second, unique
    employee, but if you only want to change the employee's name
    everywhere, consider using \l {QExplicitlySharedDataPointer}
    {explicit sharing} in the \c Employee class instead of implicit
    sharing.

    If you declare the \e {d pointer} in the \c Employee class to be
    \c {QExplicitlySharedDataPointer<EmployeeData>}, then explicit
    sharing is used and \e{copy on write} operations are not performed
    automatically (i.e. detach() is not called in non-const
    functions). In that case, after \c {e1.setName("Hans Holbein")}, the
    employee's name has been changed, but both e1 and e2 still refer to
    the same instance of \c EmployeeData, so there is only one employee
    with ID 1001.

    In the member function documentation, \e{d pointer} always refers
    to the internal pointer to the shared data object.

    \sa QSharedData, QExplicitlySharedDataPointer, QScopedPointer, QSharedPointer
*/

/*! \typedef QSharedDataPointer::Type
    This is the type of the shared data object. The \e{d pointer}
    points to an object of this type.
 */

/*! \typedef QSharedDataPointer::pointer
  \internal
 */

/*! \fn T& QSharedDataPointer::operator*()
    Provides access to the shared data object's members.
    This function calls detach().
*/

/*! \fn const T& QSharedDataPointer::operator*() const
    Provides const access to the shared data object's members.
    This function does \e not call detach().
*/

/*! \fn T* QSharedDataPointer::operator->()
    Provides access to the shared data object's members.
    This function calls detach().
*/

/*! \fn const T* QSharedDataPointer::operator->() const
    Provides const access to the shared data object's members.
    This function does \e not call detach().
*/

/*! \fn QSharedDataPointer::operator T*()
    Returns a pointer to the shared data object.
    This function calls detach().

    \sa data(), constData()
*/

/*! \fn QSharedDataPointer::operator const T*() const
    Returns a pointer to the shared data object.
    This function does \e not call detach().
*/

/*! \fn T* QSharedDataPointer::data()
    Returns a pointer to the shared data object.
    This function calls detach().

    \sa constData()
*/

/*! \fn const T* QSharedDataPointer::data() const
    Returns a pointer to the shared data object.
    This function does \e not call detach().
*/

/*! \fn const T* QSharedDataPointer::constData() const
    Returns a const pointer to the shared data object.
    This function does \e not call detach().

    \sa data()
*/

/*! \fn void QSharedDataPointer::swap(QSharedDataPointer &other)
  Swap this instance's shared data pointer with the shared
  data pointer in \a other.
 */

/*! \fn bool QSharedDataPointer::operator==(const QSharedDataPointer<T>& other) const
    Returns true if \a other and \e this have the same \e{d pointer}.
    This function does \e not call detach().
*/

/*! \fn bool QSharedDataPointer::operator!=(const QSharedDataPointer<T>& other) const
    Returns true if \a other and \e this do \e not have the same
    \e{d pointer}. This function does \e not call detach().
*/

/*! \fn QSharedDataPointer::QSharedDataPointer()
    Constructs a QSharedDataPointer initialized with a null \e{d pointer}.
*/

/*! \fn QSharedDataPointer::~QSharedDataPointer()
    Decrements the reference count of the shared data object.
    If the reference count becomes 0, the shared data object
    is deleted. \e This is then destroyed.
*/

/*! \fn QSharedDataPointer::QSharedDataPointer(T* sharedData)
    Constructs a QSharedDataPointer with \e{d pointer} set to
    \a sharedData and increments \a{sharedData}'s reference count.
*/

/*! \fn QSharedDataPointer::QSharedDataPointer(const QSharedDataPointer<T>& other)
    Sets the \e{d pointer} of \e this to the \e{d pointer} in
    \a other and increments the reference count of the shared
    data object.
*/

/*! \fn QSharedDataPointer<T>& QSharedDataPointer::operator=(const QSharedDataPointer<T>& other)
    Sets the \e{d pointer} of \e this to the \e{d pointer} of
    \a other and increments the reference count of the shared
    data object. The reference count of the old shared data
    object of \e this is decremented.  If the reference count
    of the old shared data object becomes 0, the old shared
    data object is deleted.
*/

/*! \fn QSharedDataPointer& QSharedDataPointer::operator=(T* sharedData)
    Sets the \e{d pointer} og \e this to \a sharedData and increments
    \a{sharedData}'s reference count. The reference count of the old
    shared data object of \e this is decremented.  If the reference
    count of the old shared data object becomes 0, the old shared data
    object is deleted.
*/

/*! \fn bool QSharedDataPointer::operator!() const
    Returns true if the \e{d pointer} of \e this is null.
*/

/*! \fn void QSharedDataPointer::detach()
    If the shared data object's reference count is greater than 1, this
    function creates a deep copy of the shared data object and sets the
    \e{d pointer} of \e this to the copy.

    This function is called automatically by non-const member
    functions of QSharedDataPointer if \e{copy on write} is
    required. You don't need to call it yourself.
*/

/*! \fn T *QSharedDataPointer::clone()
    \since 4.5

    Creates and returns a deep copy of the current data. This function
    is called by detach() when the reference count is greater than 1 in
    order to create the new copy. This function uses the \e {operator
    new} and calls the copy constructor of the type T.

    This function is provided so that you may support "virtual copy
    constructors" for your own types. In order to so, you should declare
    a template-specialization of this function for your own type, like
    the example below:

    \code
      template<>
      EmployeeData *QSharedDataPointer<EmployeeData>::clone()
      {
          return d->clone();
      }
    \endcode

    In the example above, the template specialization for the clone()
    function calls the \e {EmployeeData::clone()} virtual function. A
    class derived from EmployeeData could override that function and
    return the proper polymorphic type.
*/

/*! 
    \class QExplicitlySharedDataPointer
    \brief The QExplicitlySharedDataPointer class represents a pointer to an explicitly shared object.
    \since 4.4
    \reentrant

    QExplicitlySharedDataPointer\<T\> makes writing your own explicitly
    shared classes easy. QExplicitlySharedDataPointer implements
    \l {thread-safe} reference counting, ensuring that adding
    QExplicitlySharedDataPointers to your \l {reentrant} classes won't
    make them non-reentrant.

    Except for one big difference, QExplicitlySharedDataPointer is just
    like QSharedDataPointer. The big difference is that member functions
    of QExplicitlySharedDataPointer \e{do not} do the automatic
    \e{copy on write} operation (detach()) that non-const members of
    QSharedDataPointer do before allowing the shared data object to be
    modified. There is a detach() function available, but if you really
    want to detach(), you have to call it yourself. This means that
    QExplicitlySharedDataPointers behave like regular C++ pointers,
    except that by doing reference counting and not deleting the shared
    data object until the reference count is 0, they avoid the dangling
    pointer problem.

    It is instructive to compare QExplicitlySharedDataPointer with
    QSharedDataPointer by way of an example. Consider the \l {Employee
    example} in QSharedDataPointer, modified to use explicit sharing as
    explained in the discussion \l {Implicit vs Explicit Sharing}.

    Note that if you use this class but find you are calling detach() a
    lot, you probably should be using QSharedDataPointer instead.

    In the member function documentation, \e{d pointer} always refers
    to the internal pointer to the shared data object.

    \sa QSharedData, QSharedDataPointer
*/

/*! \fn T& QExplicitlySharedDataPointer::operator*() const
    Provides access to the shared data object's members.
*/

/*! \fn T* QExplicitlySharedDataPointer::operator->()
    Provides access to the shared data object's members.
*/

/*! \fn const T* QExplicitlySharedDataPointer::operator->() const
    Provides const access to the shared data object's members.
*/

/*! \fn T* QExplicitlySharedDataPointer::data() const
    Returns a pointer to the shared data object.
*/

/*! \fn const T* QExplicitlySharedDataPointer::constData() const
    Returns a const pointer to the shared data object.

    \sa data()
*/

/*! \fn void QExplicitlySharedDataPointer::swap(QExplicitlySharedDataPointer &other)
  Swap this instance's explicitly shared data pointer with
  the explicitly shared data pointer in \a other.
 */

/*! \fn bool QExplicitlySharedDataPointer::operator==(const QExplicitlySharedDataPointer<T>& other) const
    Returns true if \a other and \e this have the same \e{d pointer}.
*/

/*! \fn bool QExplicitlySharedDataPointer::operator==(const T* ptr) const
    Returns true if the \e{d pointer} of \e this is \a ptr.
 */

/*! \fn bool QExplicitlySharedDataPointer::operator!=(const QExplicitlySharedDataPointer<T>& other) const
    Returns true if \a other and \e this do \e not have the same
    \e{d pointer}.
*/

/*! \fn bool QExplicitlySharedDataPointer::operator!=(const T* ptr) const
    Returns true if the \e{d pointer} of \e this is \e not \a ptr.
 */

/*! \fn QExplicitlySharedDataPointer::QExplicitlySharedDataPointer()
    Constructs a QExplicitlySharedDataPointer initialized with a null
    \e{d pointer}.
*/

/*! \fn QExplicitlySharedDataPointer::~QExplicitlySharedDataPointer()
    Decrements the reference count of the shared data object.
    If the reference count becomes 0, the shared data object
    is deleted. \e This is then destroyed.
*/

/*! \fn QExplicitlySharedDataPointer::QExplicitlySharedDataPointer(T* sharedData)
    Constructs a QExplicitlySharedDataPointer with \e{d pointer}
    set to \a sharedData and increments \a{sharedData}'s reference
    count.
*/

/*! \fn QExplicitlySharedDataPointer::QExplicitlySharedDataPointer(const QExplicitlySharedDataPointer<T>& other)
    This standard copy constructor sets the \e {d pointer} of \e this to
    the \e {d pointer} in \a other and increments the reference count of
    the shared data object.
*/

/*! \fn QExplicitlySharedDataPointer::QExplicitlySharedDataPointer(const QExplicitlySharedDataPointer<X>& other)
    This copy constructor is different in that it allows \a other to be
    a different type of explicitly shared data pointer but one that has
    a compatible shared data object. It performs a static cast of the
    \e{d pointer} in \a other and sets the \e {d pointer} of \e this to
    the converted \e{d pointer}. It increments the reference count of
    the shared data object.
*/

/*! \fn QExplicitlySharedDataPointer<T>& QExplicitlySharedDataPointer::operator=(const QExplicitlySharedDataPointer<T>& other)
    Sets the \e{d pointer} of \e this to the \e{d pointer} of
    \a other and increments the reference count of the shared
    data object. The reference count of the old shared data
    object of \e this is decremented.  If the reference count
    of the old shared data object becomes 0, the old shared
    data object is deleted.
*/

/*! \fn QExplicitlySharedDataPointer& QExplicitlySharedDataPointer::operator=(T* sharedData)
    Sets the \e{d pointer} of \e this to \a sharedData and
    increments \a{sharedData}'s reference count. The reference
    count of the old shared data object of \e this is decremented.
    If the reference count of the old shared data object becomes
    0, the old shared data object is deleted.
*/

/*! \fn void QExplicitlySharedDataPointer::reset()
    Resets \e this to be null. i.e., this function sets the
    \e{d pointer} of \e this to 0, but first it decrements
    the reference count of the shared data object and deletes
    the shared data object if the reference count became 0.
 */

/*! \fn QExplicitlySharedDataPointer::operator bool () const
    Returns true if the \e{d pointer} of \e this is \e not null.
 */

/*! \fn bool QExplicitlySharedDataPointer::operator!() const
    Returns true if the \e{d pointer} of \e this is null.
*/

/*! \fn void QExplicitlySharedDataPointer::detach()
    If the shared data object's reference count is greater than 1, this
    function creates a deep copy of the shared data object and sets the
    \e{d pointer} of \e this to the copy.

    Because QExplicitlySharedDataPointer does not do the automatic
    \e{copy on write} operations that members of QSharedDataPointer do,
    detach() is \e not called automatically anywhere in the member
    functions of this class. If you find that you are calling detach()
    everywhere in your code, consider using QSharedDataPointer instead.
*/

/*! \fn T *QExplicitlySharedDataPointer::clone()
    \since 4.5

    Creates and returns a deep copy of the current data. This function
    is called by detach() when the reference count is greater than 1 in
    order to create the new copy. This function uses the \e {operator
    new} and calls the copy constructor of the type T.

    See QSharedDataPointer::clone() for an explanation of how to use it.
*/

/*!
    \typedef QExplicitlySharedDataPointer::Type

    This is the type of the shared data object. The \e{d pointer}
    points to an object of this type.
*/

/*! \typedef QExplicitlySharedDataPointer::pointer
  \internal
 */

QT_END_NAMESPACE
