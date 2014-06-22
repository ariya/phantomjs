/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

public class Simple
{
    int a;
    String b;
    final int c = 12345;

    void func1()
    {
    }

    void func2()
    {
        return 123;
    }

    void func3() {
        return 123;
    }

    void func4() { return 123; }

    void func5()
    {
        /* comment */
    }

    /*
      void funcInsideComment()
      {
      }
    */

    void func6(int a)
    {
    }

    void func7(int a, int b, int c)
    {
    }

    void func8(int a, int b,
               int c, int d
               , int e, int f)
    {
    }

    void func9
        (int a, int b)
    {
    }

    LinkedList func10()
    {
    }

    void funcOverloaded()
    {
    }

    void funcOverloaded(int a)
    {
    }

    void funcOverloaded(float a)
    {
    }

    static void func11()
    {
    }

    public void func12()
    {
    }

    protected void func13()
    {
    }

    private void func14()
    {
    }

    static void func15()
    {
    }

    final void func16()
    {
    }

    abstract void func17()
    {
    }

    synchronized void func18()
    {
    }

    final static public synchronized void func19()
    {
    }

    void func20() throws IOException
    {
    }

    void func21() throws IOException, ArithmeticException
    {
    }
}

import java.util.*;
import java.math.*;

class Derived1 extends Base
{
    public Derived1()
    {
    }

    public func22()
    {
    }
}

interface Interface1
{
}

interface Interface2
{
    int a;

    void func23()
    {
    }
}

class Derived2 extends Base interface Interface1, Interface2
{
    public Derived2()
    {
    }

    public func23()
    {
    }
}
