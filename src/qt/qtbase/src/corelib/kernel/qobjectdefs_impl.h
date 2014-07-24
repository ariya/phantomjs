/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013 Olivier Goffart <ogoffart@woboq.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef Q_QDOC

#ifndef QOBJECTDEFS_H
#error Do not include qobjectdefs_impl.h directly
#endif

#if 0
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

QT_BEGIN_NAMESPACE


namespace QtPrivate {
    template <typename T> struct RemoveRef { typedef T Type; };
    template <typename T> struct RemoveRef<T&> { typedef T Type; };
    template <typename T> struct RemoveConstRef { typedef T Type; };
    template <typename T> struct RemoveConstRef<const T&> { typedef T Type; };

    /*
       The following List classes are used to help to handle the list of arguments.
       It follow the same principles as the lisp lists.
       List_Left<L,N> take a list and a number as a parameter and returns (via the Value typedef,
       the list composed of the first N element of the list
     */
#ifndef Q_COMPILER_VARIADIC_TEMPLATES
    template <typename Head, typename Tail> struct List { typedef Head Car; typedef Tail Cdr; };
    template <typename L, int N> struct List_Left { typedef List<typename L::Car, typename List_Left<typename L::Cdr, N - 1>::Value > Value; };
    template <typename L> struct List_Left<L,0> { typedef void Value; };
#else
    // With variadic template, lists are represented using a variadic template argument instead of the lisp way
    template <typename...> struct List {};
    template <typename Head, typename... Tail> struct List<Head, Tail...> { typedef Head Car; typedef List<Tail...> Cdr; };
    template <typename, typename> struct List_Append;
    template <typename... L1, typename...L2> struct List_Append<List<L1...>, List<L2...>> { typedef List<L1..., L2...> Value; };
    template <typename L, int N> struct List_Left {
        typedef typename List_Append<List<typename L::Car>,typename List_Left<typename L::Cdr, N - 1>::Value>::Value Value;
    };
    template <typename L> struct List_Left<L, 0> { typedef List<> Value; };
#endif
    // List_Select<L,N> returns (via typedef Value) the Nth element of the list L
    template <typename L, int N> struct List_Select { typedef typename List_Select<typename L::Cdr, N - 1>::Value Value; };
    template <typename L> struct List_Select<L,0> { typedef typename L::Car Value; };

    /*
       trick to set the return value of a slot that works even if the signal or the slot returns void
       to be used like     function(), ApplyReturnValue<ReturnType>(&return_value)
       if function() returns a value, the operator,(T, ApplyReturnValue<ReturnType>) is called, but if it
       returns void, the builtin one is used without an error.
    */
    template <typename T>
    struct ApplyReturnValue {
        void *data;
        explicit ApplyReturnValue(void *data_) : data(data_) {}
    };
    template<typename T, typename U>
    void operator,(const T &value, const ApplyReturnValue<U> &container) {
        if (container.data)
            *reinterpret_cast<U*>(container.data) = value;
    }
#ifdef Q_COMPILER_RVALUE_REFS
    template<typename T, typename U>
    void operator,(T &&value, const ApplyReturnValue<U> &container) {
        if (container.data)
            *reinterpret_cast<U*>(container.data) = value;
    }
#endif
    template<typename T>
    void operator,(T, const ApplyReturnValue<void> &) {}


    /*
      The FunctionPointer<Func> struct is a type trait for function pointer.
        - ArgumentCount  is the number of argument, or -1 if it is unknown
        - the Object typedef is the Object of a pointer to member function
        - the Arguments typedef is the list of argument (in a QtPrivate::List)
        - the Function typedef is an alias to the template parameter Func
        - the call<Args, R>(f,o,args) method is used to call that slot
            Args is the list of argument of the signal
            R is the return type of the signal
            f is the function pointer
            o is the receiver object
            and args is the array of pointer to arguments, as used in qt_metacall

       The Functor<Func,N> struct is the helper to call a functor of N argument.
       its call function is the same as the FunctionPointer::call function.
     */
#ifndef Q_COMPILER_VARIADIC_TEMPLATES
    template<typename Func> struct FunctionPointer { enum {ArgumentCount = -1, IsPointerToMemberFunction = false}; };
    //Pointers to member functions
    template<class Obj, typename Ret> struct FunctionPointer<Ret (Obj::*) ()>
    {
        typedef Obj Object;
        typedef void Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) ();
        enum {ArgumentCount = 0, IsPointerToMemberFunction = true};
        template <typename Args, typename R>
        static void call(Function f, Obj *o, void **arg) { (o->*f)(), ApplyReturnValue<R>(arg[0]); }
    };
    template<class Obj, typename Ret, typename Arg1> struct FunctionPointer<Ret (Obj::*) (Arg1)>
    {
        typedef Obj Object;
        typedef List<Arg1, void> Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) (Arg1);
        enum {ArgumentCount = 1, IsPointerToMemberFunction = true};
        template <typename Args, typename R>
        static void call(Function f, Obj *o, void **arg) {
            (o->*f)((*reinterpret_cast<typename RemoveRef<typename Args::Car>::Type *>(arg[1]))), ApplyReturnValue<R>(arg[0]);
        }
    };
    template<class Obj, typename Ret, typename Arg1, typename Arg2> struct FunctionPointer<Ret (Obj::*) (Arg1, Arg2)>
    {
        typedef Obj Object;
        typedef List<Arg1, List<Arg2, void> >  Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) (Arg1, Arg2);
        enum {ArgumentCount = 2, IsPointerToMemberFunction = true};
        template <typename Args, typename R>
        static void call(Function f, Obj *o, void **arg) {
            (o->*f)( *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 1>::Value>::Type *>(arg[2])), ApplyReturnValue<R>(arg[0]);
        }
    };
    template<class Obj, typename Ret, typename Arg1, typename Arg2, typename Arg3> struct FunctionPointer<Ret (Obj::*) (Arg1, Arg2, Arg3)>
    {
        typedef Obj Object;
        typedef List<Arg1, List<Arg2, List<Arg3, void> > >  Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) (Arg1, Arg2, Arg3);
        enum {ArgumentCount = 3, IsPointerToMemberFunction = true};
        template <typename Args, typename R>
        static void call(Function f, Obj *o, void **arg) {
            (o->*f)( *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 1>::Value>::Type *>(arg[2]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 2>::Value>::Type *>(arg[3])), ApplyReturnValue<R>(arg[0]);
        }
    };
    template<class Obj, typename Ret, typename Arg1, typename Arg2, typename Arg3, typename Arg4> struct FunctionPointer<Ret (Obj::*) (Arg1, Arg2, Arg3, Arg4)>
    {
        typedef Obj Object;
        typedef List<Arg1, List<Arg2, List<Arg3, List<Arg4, void> > > >  Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) (Arg1, Arg2, Arg3, Arg4);
        enum {ArgumentCount = 4, IsPointerToMemberFunction = true};
        template <typename Args, typename R>
        static void call(Function f, Obj *o, void **arg) {
            (o->*f)( *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 1>::Value>::Type *>(arg[2]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 2>::Value>::Type *>(arg[3]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 3>::Value>::Type *>(arg[4])), ApplyReturnValue<R>(arg[0]);
        }
    };
    template<class Obj, typename Ret, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> struct FunctionPointer<Ret (Obj::*) (Arg1, Arg2, Arg3, Arg4, Arg5)>
    {
        typedef Obj Object;
        typedef List<Arg1, List<Arg2, List<Arg3, List<Arg4, List<Arg5, void> > > > >  Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) (Arg1, Arg2, Arg3, Arg4, Arg5);
        enum {ArgumentCount = 5, IsPointerToMemberFunction = true};
        template <typename Args, typename R>
        static void call(Function f, Obj *o, void **arg) {
            (o->*f)( *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 1>::Value>::Type *>(arg[2]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 2>::Value>::Type *>(arg[3]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 3>::Value>::Type *>(arg[4]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 4>::Value>::Type *>(arg[5])), ApplyReturnValue<R>(arg[0]);
        }
    };
    template<class Obj, typename Ret, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
    struct FunctionPointer<Ret (Obj::*) (Arg1, Arg2, Arg3, Arg4, Arg5, Arg6)>
    {
        typedef Obj Object;
        typedef List<Arg1, List<Arg2, List<Arg3, List<Arg4, List<Arg5, List<Arg6, void> > > > > >  Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) (Arg1, Arg2, Arg3, Arg4, Arg5, Arg6);
        enum {ArgumentCount = 6, IsPointerToMemberFunction = true};
        template <typename Args, typename R>
        static void call(Function f, Obj *o, void **arg) {
            (o->*f)( *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 1>::Value>::Type *>(arg[2]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 2>::Value>::Type *>(arg[3]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 3>::Value>::Type *>(arg[4]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 4>::Value>::Type *>(arg[5]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 5>::Value>::Type *>(arg[6])), ApplyReturnValue<R>(arg[0]);
        }
    };

    //Pointers to const member functions
    template<class Obj, typename Ret> struct FunctionPointer<Ret (Obj::*) () const>
    {
        typedef Obj Object;
        typedef void Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) () const;
        enum {ArgumentCount = 0, IsPointerToMemberFunction = true};
        template <typename Args, typename R>
        static void call(Function f, Obj *o, void **arg) { (o->*f)(), ApplyReturnValue<R>(arg[0]); }
    };
    template<class Obj, typename Ret, typename Arg1> struct FunctionPointer<Ret (Obj::*) (Arg1) const>
    {
        typedef Obj Object;
        typedef List<Arg1, void> Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) (Arg1) const;
        enum {ArgumentCount = 1, IsPointerToMemberFunction = true};
        template <typename Args, typename R>
        static void call(Function f, Obj *o, void **arg) {
            (o->*f)((*reinterpret_cast<typename RemoveRef<typename Args::Car>::Type *>(arg[1]))), ApplyReturnValue<R>(arg[0]);
        }
    };
    template<class Obj, typename Ret, typename Arg1, typename Arg2> struct FunctionPointer<Ret (Obj::*) (Arg1, Arg2) const>
    {
        typedef Obj Object;
        typedef List<Arg1, List<Arg2, void> >  Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) (Arg1, Arg2) const;
        enum {ArgumentCount = 2, IsPointerToMemberFunction = true};
        template <typename Args, typename R>
        static void call(Function f, Obj *o, void **arg) {
            (o->*f)( *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 1>::Value>::Type *>(arg[2])), ApplyReturnValue<R>(arg[0]);
        }
    };
    template<class Obj, typename Ret, typename Arg1, typename Arg2, typename Arg3> struct FunctionPointer<Ret (Obj::*) (Arg1, Arg2, Arg3) const>
    {
        typedef Obj Object;
        typedef List<Arg1, List<Arg2, List<Arg3, void> > >  Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) (Arg1, Arg2, Arg3) const;
        enum {ArgumentCount = 3, IsPointerToMemberFunction = true};
        template <typename Args, typename R>
        static void call(Function f, Obj *o, void **arg) {
            (o->*f)( *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 1>::Value>::Type *>(arg[2]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 2>::Value>::Type *>(arg[3])), ApplyReturnValue<R>(arg[0]);
        }
    };
    template<class Obj, typename Ret, typename Arg1, typename Arg2, typename Arg3, typename Arg4> struct FunctionPointer<Ret (Obj::*) (Arg1, Arg2, Arg3, Arg4) const>
    {
        typedef Obj Object;
        typedef List<Arg1, List<Arg2, List<Arg3, List<Arg4, void> > > >  Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) (Arg1, Arg2, Arg3, Arg4) const;
        enum {ArgumentCount = 4, IsPointerToMemberFunction = true};
        template <typename Args, typename R>
        static void call(Function f, Obj *o, void **arg) {
            (o->*f)( *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 1>::Value>::Type *>(arg[2]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 2>::Value>::Type *>(arg[3]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 3>::Value>::Type *>(arg[4])), ApplyReturnValue<R>(arg[0]);
        }
    };
    template<class Obj, typename Ret, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> struct FunctionPointer<Ret (Obj::*) (Arg1, Arg2, Arg3, Arg4, Arg5) const>
    {
        typedef Obj Object;
        typedef List<Arg1, List<Arg2, List<Arg3, List<Arg4, List<Arg5, void> > > > >  Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) (Arg1, Arg2, Arg3, Arg4, Arg5) const;
        enum {ArgumentCount = 5, IsPointerToMemberFunction = true};
        template <typename Args, typename R>
        static void call(Function f, Obj *o, void **arg) {
            (o->*f)( *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 1>::Value>::Type *>(arg[2]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 2>::Value>::Type *>(arg[3]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 3>::Value>::Type *>(arg[4]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 4>::Value>::Type *>(arg[5])), ApplyReturnValue<R>(arg[0]);
        }
    };
    template<class Obj, typename Ret, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
    struct FunctionPointer<Ret (Obj::*) (Arg1, Arg2, Arg3, Arg4, Arg5, Arg6) const>
    {
        typedef Obj Object;
        typedef List<Arg1, List<Arg2, List<Arg3, List<Arg4, List<Arg5, List<Arg6, void> > > > > >  Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) (Arg1, Arg2, Arg3, Arg4, Arg5, Arg6) const;
        enum {ArgumentCount = 6, IsPointerToMemberFunction = true};
        template <typename Args, typename R>
        static void call(Function f, Obj *o, void **arg) {
            (o->*f)( *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 1>::Value>::Type *>(arg[2]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 2>::Value>::Type *>(arg[3]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 3>::Value>::Type *>(arg[4]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 4>::Value>::Type *>(arg[5]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 5>::Value>::Type *>(arg[6])), ApplyReturnValue<R>(arg[0]);
        }
    };

    //Static functions
    template<typename Ret> struct FunctionPointer<Ret (*) ()>
    {
        typedef void Arguments;
        typedef Ret (*Function) ();
        typedef Ret ReturnType;
        enum {ArgumentCount = 0, IsPointerToMemberFunction = false};
        template <typename Args, typename R>
        static void call(Function f, void *, void **arg) { f(), ApplyReturnValue<R>(arg[0]); }
    };
    template<typename Ret, typename Arg1> struct FunctionPointer<Ret (*) (Arg1)>
    {
        typedef List<Arg1, void> Arguments;
        typedef Ret ReturnType;
        typedef Ret (*Function) (Arg1);
        enum {ArgumentCount = 1, IsPointerToMemberFunction = false};
        template <typename Args, typename R>
        static void call(Function f, void *, void **arg)
        { f(*reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1])), ApplyReturnValue<R>(arg[0]); }
    };
    template<typename Ret, typename Arg1, typename Arg2> struct FunctionPointer<Ret (*) (Arg1, Arg2)>
    {
        typedef List<Arg1, List<Arg2, void> > Arguments;
        typedef Ret ReturnType;
        typedef Ret (*Function) (Arg1, Arg2);
        enum {ArgumentCount = 2, IsPointerToMemberFunction = false};
        template <typename Args, typename R>
        static void call(Function f, void *, void **arg) {
            f(*reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1]),
              *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 1>::Value>::Type *>(arg[2])), ApplyReturnValue<R>(arg[0]); }
    };
    template<typename Ret, typename Arg1, typename Arg2, typename Arg3> struct FunctionPointer<Ret (*) (Arg1, Arg2, Arg3)>
    {
        typedef List<Arg1, List<Arg2, List<Arg3, void> > >  Arguments;
        typedef Ret ReturnType;
        typedef Ret (*Function) (Arg1, Arg2, Arg3);
        enum {ArgumentCount = 3, IsPointerToMemberFunction = false};
        template <typename Args, typename R>
        static void call(Function f, void *, void **arg) {
            f(       *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 1>::Value>::Type *>(arg[2]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 2>::Value>::Type *>(arg[3])), ApplyReturnValue<R>(arg[0]);
        }
    };
    template<typename Ret, typename Arg1, typename Arg2, typename Arg3, typename Arg4> struct FunctionPointer<Ret (*) (Arg1, Arg2, Arg3, Arg4)>
    {
        typedef List<Arg1, List<Arg2, List<Arg3, List<Arg4, void> > > >  Arguments;
        typedef Ret ReturnType;
        typedef Ret (*Function) (Arg1, Arg2, Arg3, Arg4);
        enum {ArgumentCount = 4, IsPointerToMemberFunction = false};
        template <typename Args, typename R>
        static void call(Function f, void *, void **arg) {
            f(       *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 1>::Value>::Type *>(arg[2]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 2>::Value>::Type *>(arg[3]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 3>::Value>::Type *>(arg[4])), ApplyReturnValue<R>(arg[0]);
        }
    };
    template<typename Ret, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> struct FunctionPointer<Ret (*) (Arg1, Arg2, Arg3, Arg4, Arg5)>
    {
        typedef List<Arg1, List<Arg2, List<Arg3,
        List<Arg4, List<Arg5, void > > > > >  Arguments;
        typedef Ret ReturnType;
        typedef Ret (*Function) (Arg1, Arg2, Arg3, Arg4, Arg5);
        enum {ArgumentCount = 5, IsPointerToMemberFunction = false};
        template <typename Args, typename R>
        static void call(Function f, void *, void **arg) {
            f(       *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 1>::Value>::Type *>(arg[2]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 2>::Value>::Type *>(arg[3]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 3>::Value>::Type *>(arg[4]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 4>::Value>::Type *>(arg[5])), ApplyReturnValue<R>(arg[0]);
        }
    };
    template<typename Ret, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6> struct FunctionPointer<Ret (*) (Arg1, Arg2, Arg3, Arg4, Arg5, Arg6)>
    {
        typedef List<Arg1, List<Arg2, List<Arg3, List<Arg4, List<Arg5, List<Arg6, void> > > > > >  Arguments;
        typedef Ret ReturnType;
        typedef Ret (*Function) (Arg1, Arg2, Arg3, Arg4, Arg5, Arg6);
        enum {ArgumentCount = 6, IsPointerToMemberFunction = false};
        template <typename Args, typename R>
        static void call(Function f, void *, void **arg) {
            f(       *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 1>::Value>::Type *>(arg[2]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 2>::Value>::Type *>(arg[3]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 3>::Value>::Type *>(arg[4]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 4>::Value>::Type *>(arg[5]),
                     *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 5>::Value>::Type *>(arg[6])), ApplyReturnValue<R>(arg[0]);
        }
    };

    //Functors
    template<typename F, int N> struct Functor;
    template<typename Function> struct Functor<Function, 0>
    {
        template <typename Args, typename R>
        static void call(Function &f, void *, void **arg) { f(), ApplyReturnValue<R>(arg[0]); }
    };
    template<typename Function> struct Functor<Function, 1>
    {
        template <typename Args, typename R>
        static void call(Function &f, void *, void **arg) {
            f(*reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1])), ApplyReturnValue<R>(arg[0]);
        }
    };
    template<typename Function> struct Functor<Function, 2>
    {
        template <typename Args, typename R>
        static void call(Function &f, void *, void **arg) {
            f( *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1]),
               *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 1>::Value>::Type *>(arg[2])), ApplyReturnValue<R>(arg[0]);
        }
    };
    template<typename Function> struct Functor<Function, 3>
    {
        template <typename Args, typename R>
        static void call(Function &f, void *, void **arg) {
            f( *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1]),
               *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 1>::Value>::Type *>(arg[2]),
               *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 2>::Value>::Type *>(arg[3])), ApplyReturnValue<R>(arg[0]);
        }
    };
    template<typename Function> struct Functor<Function, 4>
    {
        template <typename Args, typename R>
        static void call(Function &f, void *, void **arg) {
            f( *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1]),
               *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 1>::Value>::Type *>(arg[2]),
               *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 2>::Value>::Type *>(arg[3]),
               *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 3>::Value>::Type *>(arg[4])), ApplyReturnValue<R>(arg[0]);
        }
    };
    template<typename Function> struct Functor<Function, 5>
    {
        template <typename Args, typename R>
        static void call(Function &f, void *, void **arg) {
            f( *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1]),
               *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 1>::Value>::Type *>(arg[2]),
               *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 2>::Value>::Type *>(arg[3]),
               *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 3>::Value>::Type *>(arg[4]),
               *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 4>::Value>::Type *>(arg[5])), ApplyReturnValue<R>(arg[0]);
        }
    };
    template<typename Function> struct Functor<Function, 6>
    {
        template <typename Args, typename R>
        static void call(Function &f, void *, void **arg) {
            f( *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 0>::Value>::Type *>(arg[1]),
               *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 1>::Value>::Type *>(arg[2]),
               *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 2>::Value>::Type *>(arg[3]),
               *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 3>::Value>::Type *>(arg[4]),
               *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 4>::Value>::Type *>(arg[5]),
               *reinterpret_cast<typename RemoveRef<typename List_Select<Args, 5>::Value>::Type *>(arg[6])), ApplyReturnValue<R>(arg[0]);
        }
    };
#else
    template <int...> struct IndexesList {};
    template <typename IndexList, int Right> struct IndexesAppend;
    template <int... Left, int Right> struct IndexesAppend<IndexesList<Left...>, Right>
    { typedef IndexesList<Left..., Right> Value; };
    template <int N> struct Indexes
    { typedef typename IndexesAppend<typename Indexes<N - 1>::Value, N - 1>::Value Value; };
    template <> struct Indexes<0> { typedef IndexesList<> Value; };
    template<typename Func> struct FunctionPointer { enum {ArgumentCount = -1, IsPointerToMemberFunction = false}; };

    template <typename, typename, typename, typename> struct FunctorCall;
    template <int... II, typename... SignalArgs, typename R, typename Function>
    struct FunctorCall<IndexesList<II...>, List<SignalArgs...>, R, Function> {
        static void call(Function f, void **arg) {
            f((*reinterpret_cast<typename RemoveRef<SignalArgs>::Type *>(arg[II+1]))...), ApplyReturnValue<R>(arg[0]);
        }
    };
    template <int... II, typename... SignalArgs, typename R, typename... SlotArgs, typename SlotRet, class Obj>
    struct FunctorCall<IndexesList<II...>, List<SignalArgs...>, R, SlotRet (Obj::*)(SlotArgs...)> {
        static void call(SlotRet (Obj::*f)(SlotArgs...), Obj *o, void **arg) {
            (o->*f)((*reinterpret_cast<typename RemoveRef<SignalArgs>::Type *>(arg[II+1]))...), ApplyReturnValue<R>(arg[0]);
        }
    };
    template <int... II, typename... SignalArgs, typename R, typename... SlotArgs, typename SlotRet, class Obj>
    struct FunctorCall<IndexesList<II...>, List<SignalArgs...>, R, SlotRet (Obj::*)(SlotArgs...) const> {
        static void call(SlotRet (Obj::*f)(SlotArgs...) const, Obj *o, void **arg) {
            (o->*f)((*reinterpret_cast<typename RemoveRef<SignalArgs>::Type *>(arg[II+1]))...), ApplyReturnValue<R>(arg[0]);
        }
    };

    template<class Obj, typename Ret, typename... Args> struct FunctionPointer<Ret (Obj::*) (Args...)>
    {
        typedef Obj Object;
        typedef List<Args...>  Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) (Args...);
        enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = true};
        template <typename SignalArgs, typename R>
        static void call(Function f, Obj *o, void **arg) {
            FunctorCall<typename Indexes<ArgumentCount>::Value, SignalArgs, R, Function>::call(f, o, arg);
        }
    };
    template<class Obj, typename Ret, typename... Args> struct FunctionPointer<Ret (Obj::*) (Args...) const>
    {
        typedef Obj Object;
        typedef List<Args...>  Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) (Args...) const;
        enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = true};
        template <typename SignalArgs, typename R>
        static void call(Function f, Obj *o, void **arg) {
            FunctorCall<typename Indexes<ArgumentCount>::Value, SignalArgs, R, Function>::call(f, o, arg);
        }
    };

    template<typename Ret, typename... Args> struct FunctionPointer<Ret (*) (Args...)>
    {
        typedef List<Args...> Arguments;
        typedef Ret ReturnType;
        typedef Ret (*Function) (Args...);
        enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = false};
        template <typename SignalArgs, typename R>
        static void call(Function f, void *, void **arg) {
            FunctorCall<typename Indexes<ArgumentCount>::Value, SignalArgs, R, Function>::call(f, arg);
        }
    };

    template<typename Function, int N> struct Functor
    {
        template <typename SignalArgs, typename R>
        static void call(Function &f, void *, void **arg) {
            FunctorCall<typename Indexes<N>::Value, SignalArgs, R, Function>::call(f, arg);
        }
    };
#endif

    /*
       Logic that check if the arguments of the slot matches the argument of the signal.
       To be used like this:
       Q_STATIC_ASSERT(CheckCompatibleArguments<FunctionPointer<Signal>::Arguments, FunctionPointer<Slot>::Arguments>::value)
    */
    template<typename A1, typename A2> struct AreArgumentsCompatible {
        static int test(const typename RemoveRef<A2>::Type&);
        static char test(...);
        static const typename RemoveRef<A1>::Type &dummy();
        enum { value = sizeof(test(dummy())) == sizeof(int) };
    };
    template<typename A1, typename A2> struct AreArgumentsCompatible<A1, A2&> { enum { value = false }; };
    template<typename A> struct AreArgumentsCompatible<A&, A&> { enum { value = true }; };
    // void as a return value
    template<typename A> struct AreArgumentsCompatible<void, A> { enum { value = true }; };
    template<typename A> struct AreArgumentsCompatible<A, void> { enum { value = true }; };
    template<> struct AreArgumentsCompatible<void, void> { enum { value = true }; };

#ifndef Q_COMPILER_VARIADIC_TEMPLATES
    template <typename List1, typename List2> struct CheckCompatibleArguments { enum { value = false }; };
    template <> struct CheckCompatibleArguments<void, void> { enum { value = true }; };
    template <typename List1> struct CheckCompatibleArguments<List1, void> { enum { value = true }; };
    template <typename Arg1, typename Arg2, typename Tail1, typename Tail2> struct CheckCompatibleArguments<List<Arg1, Tail1>, List<Arg2, Tail2> >
    {
        enum { value = AreArgumentsCompatible<typename RemoveConstRef<Arg1>::Type, typename RemoveConstRef<Arg2>::Type>::value
                    && CheckCompatibleArguments<Tail1, Tail2>::value };
    };
#else
    template <typename List1, typename List2> struct CheckCompatibleArguments { enum { value = false }; };
    template <> struct CheckCompatibleArguments<List<>, List<>> { enum { value = true }; };
    template <typename List1> struct CheckCompatibleArguments<List1, List<>> { enum { value = true }; };
    template <typename Arg1, typename Arg2, typename... Tail1, typename... Tail2>
    struct CheckCompatibleArguments<List<Arg1, Tail1...>, List<Arg2, Tail2...>>
    {
        enum { value = AreArgumentsCompatible<typename RemoveConstRef<Arg1>::Type, typename RemoveConstRef<Arg2>::Type>::value
                    && CheckCompatibleArguments<List<Tail1...>, List<Tail2...>>::value };
    };
#endif

#if defined(Q_COMPILER_DECLTYPE) && defined(Q_COMPILER_VARIADIC_TEMPLATES)
    /*
       Find the maximum number of arguments a functor object can take and be still compatible with
       the arguments from the signal.
       Value is the number of arguments, or -1 if nothing matches.
     */
    template <typename Functor, typename ArgList> struct ComputeFunctorArgumentCount;

    template <typename Functor, typename ArgList, bool Done> struct ComputeFunctorArgumentCountHelper
    { enum { Value = -1 }; };
    template <typename Functor, typename First, typename... ArgList>
    struct ComputeFunctorArgumentCountHelper<Functor, List<First, ArgList...>, false>
        : ComputeFunctorArgumentCount<Functor,
            typename List_Left<List<First, ArgList...>, sizeof...(ArgList)>::Value> {};

    template <typename Functor, typename... ArgList> struct ComputeFunctorArgumentCount<Functor, List<ArgList...>>
    {
        template <typename D> static D dummy();
        template <typename F> static auto test(F f) -> decltype(((f.operator()((dummy<ArgList>())...)), int()));
        static char test(...);
        enum {
            Ok = sizeof(test(dummy<Functor>())) == sizeof(int),
            Value = Ok ? sizeof...(ArgList) : int(ComputeFunctorArgumentCountHelper<Functor, List<ArgList...>, Ok>::Value)
        };
    };

    /* get the return type of a functor, given the signal argument list  */
    template <typename Functor, typename ArgList> struct FunctorReturnType;
    template <typename Functor, typename ... ArgList> struct FunctorReturnType<Functor, List<ArgList...>> {
        template <typename D> static D dummy();
        typedef decltype(dummy<Functor>().operator()((dummy<ArgList>())...)) Value;
    };
#endif

}

QT_END_NAMESPACE

#endif
