2002-12-03  Maciej Stachowiak  <mjs@apple.com>

        Reviewed by: Darin Adler

	- fixed Deployment build.
	
        * kjs/dtoa.cpp: Work around warnings.

2002-12-03  Maciej Stachowiak  <mjs@apple.com>

	- fixed 3114790 - Gamespot reviews pages badly mis-rendering
	because floating point numbers format wide

	Reviewed by: David Hyatt
	
	* kjs/dtoa.cpp: Imported float <--> string conversion routines
	from David M. Gay. I changed this to fix warnings and avoid
	colliding with names of standard library functions.
        * kjs/dtoa.h: Added a header I made up for dtoa.cpp
        * kjs/ustring.cpp:
        (UString::from): Use new double to string routine (kjs_strtod).
        (UString::toDouble): Use new string to double routine (kjs_dtoa).
        * JavaScriptCore.pbproj/project.pbxproj: Added new files

2002-11-27  John Sullivan  <sullivan@apple.com>

        * kjs/collector.cpp:
	removed puts("COLLECT") leftover debugging spam that was
	buggin' gramps

=== Alexander-34 ===

2002-11-26  Maciej Stachowiak  <mjs@apple.com>

	Change ActivationImp to be allocated via the garbage collector
	again instead of on the stack. This fixes the following four
	regressions but sadly it causes a 6% performance hit. It's
	probably possibly to reduce the hit a bit by being smarter about
	inlining and the way the marking list variant is implemented, but
	I'll look into that later.

	- fixed 3111500 - REGRESSION: crash in "KJS::ScopeChain::mark()" on www.posci.com
	- fixed 3111145 - REGRESSION: reproducible crash in KJS hashtable lookup at time.com
	- fixed 3110897 - REGRESSION: javascript crasher on http://bmwgallery.tripod.com/
	- fixed 3109987 - REGRESSION: Reproducible crash in KJS ObjectImp at live365.com
	
	Also:
	
	- improved DEBUG_COLLECTOR mode a bit by never giving memory back
	to the system.
	
        * kjs/collector.cpp:
        * kjs/context.h:
        * kjs/function.cpp:
        (ActivationImp::ActivationImp):
        (ActivationImp::mark):
        (ActivationImp::createArgumentsObject):
        * kjs/function.h:
        * kjs/internal.cpp:
        (ContextImp::ContextImp):
        (ContextImp::mark):
        * kjs/list.cpp:
        * kjs/list.h:
        * kjs/value.cpp:
        (Value::Value):

2002-11-26  Darin Adler  <darin@apple.com>

        * kjs/property_map.cpp:
	(PropertyMap::save): Look at the attributes the same way in the single hash entry
	case as in the actual hash table case. Change the rule for which attributes to save
	to "attributes that don't have the ReadOnly, DontEnum, or Function bit set".
        Also fix bug where saving an empty property map would leave the count set to the old value.

2002-11-26  Richard Williamson   <rjw@apple.com>

        Remove debugging code.  Could be cause of performance regresssion.
        * kjs/nodes.cpp:
        (FunctionCallNode::evaluate):

        Restire attributes correctly.
        * kjs/property_map.cpp:

2002-11-25  Richard Williamson   <rjw@apple.com>

        Use delete[] (not delete) operator to delete array.
        
        * kjs/property_map.cpp:

2002-11-25  Richard Williamson   <rjw@apple.com>

        Added debugging info.  Fixed property map save function.
        
        * kjs/nodes.cpp:
        (FunctionCallNode::evaluate):
        * kjs/property_map.cpp:

2002-11-25  Richard Williamson   <rjw@apple.com>

        Changes for back/forward.  Currently disabled.

        * kjs/property_map.cpp:
        * kjs/property_map.h:

2002-11-25  Darin Adler  <darin@apple.com>

        * kjs/property_map.cpp: Rearrange code a little bit and tweak indentation.
	This might provide a tiny speedup because we don't look at the single entry
	any more in cases where the _table pointer is non-0.

2002-11-24  Darin Adler  <darin@apple.com>

	- changed ScopeChain to not ref each item in the chain, and use
        marking instead; gains 1% on JavaScript iBench

        * kjs/context.h: Return chain by reference.
        * kjs/internal.cpp: (ContextImp::mark): Mark the scope chain.
        * kjs/interpreter.cpp: (Context::scopeChain): Return chain by reference.
        * kjs/interpreter.h: Make some Context methods inline.
        * kjs/nodes.cpp:
        (ThisNode::evaluate): Get at ContextImp directly.
        (ResolveNode::evaluateReference): Ditto.
        (VarDeclNode::evaluate): Ditto.
        (VarDeclNode::processVarDecls): Ditto.
        (FuncDeclNode::processFuncDecl): Pass ScopeChain directly to avoid copying.
        (FuncExprNode::evaluate): Ditto.
        * kjs/object.cpp: Make scope and setScope inline.
        * kjs/object.h: Make scope return a chain by reference. Make scope and
	setScope both be inline. Use a normal ScopeChain instead of NoRefScopeChain
	since they are now one and the same.
        * kjs/scope_chain.cpp: Remove all the code to ref and deref objects.
	Merge NoRefScopeChain in with ScopeChain since they both work this way now.
        * kjs/scope_chain.h: Remove NoRefScopeChain and simplify the ref counts.
	Make more functions inline.

2002-11-24  Maciej Stachowiak  <mjs@apple.com>

	- fixed 3098356 - Hard hang on movie search at www.movietickets.com
	
        * kjs/string_object.cpp:
        (StringProtoFuncImp::call): When doing a regexp replacement that
	results in an empty match, always move on to the next character
	after doing the replacement. The previous code would hit an
	infinite loop if an initial empty match was replaced with the
	empty string.

2002-11-24  Maciej Stachowiak  <mjs@apple.com>

	- fixed 3095446 - Crash on AppleScript page due to very long argument list
	
        * kjs/grammar.y: Don't try to construct the argument list in the
	right order, since that blows out the parser stack.
	* kjs/nodes.cpp:
        (ArgumentsNode::ArgumentsNode): Instead reverse the argument list
	here.
        * kjs/nodes.h: Make ArgumentsNode a friend of ArgumentListNode.
        * kjs/grammar.cpp: Updated from grammar.y.

2002-11-23  Maciej Stachowiak  <mjs@apple.com>

	- completed Darin's mostly-fix for 3037795 - Resource use
	increases when accessing very high index value in array

	The two missing pieces were handling sparse properties when
	shrinking the array, and when sorting. Thse are now both taken
	care of.
	
        * kjs/array_instance.h:
        * kjs/array_object.cpp:
        (ArrayInstanceImp::put):
        (ArrayInstanceImp::deleteProperty):
        (ArrayInstanceImp::resizeStorage):
        (ArrayInstanceImp::setLength):
        (ArrayInstanceImp::sort):
        (ArrayInstanceImp::pushUndefinedObjectsToEnd):
        * kjs/identifier.h:
        * kjs/object.h:
        * kjs/property_map.cpp:
        * kjs/property_map.h:
        * kjs/reference_list.cpp:
        (ReferenceList::append):
        (ReferenceList::length):
        * kjs/reference_list.h:
        * kjs/ustring.cpp:
        (UString::toUInt32):
        * kjs/ustring.h:

2002-11-23  Maciej Stachowiak  <mjs@apple.com>

	Numerous collector changes for a net gain of 3% on JS ibench:

	- Replaced per-block bitmap with free list.
	- Increased number of empty blocks kept around to 2.
	- Doubled block size.
	- When scanning heap in collector, skip scanning the rest of a
	block as soon as we see as many live cells as the the number of
	used cells it had originally.

	Also the following collector changes unrelated to performance:

	- Made constants `const int' instead of `static const int'.
	- Miscellaneous code cleanup.
		
        * kjs/collector.cpp:

	- Added debugging mode enabled by defining DEBUG_GC which asserts
	when a destroyed ValueImp

        * kjs/internal.cpp:
        (ContextImp::mark):
        * kjs/value.cpp:
        (Value::Value):
        * kjs/value.h:
	* kjs/config.h:
	
2002-11-22  Darin Adler  <darin@apple.com>

	- replaced List class with a vector rather than a linked list, changed it
	to use a pool of instances instead of all the nodes allocated off of the
	heap; gives 10% gain on iBench

        * kjs/list.h: Complete rewrite.
        * kjs/list.cpp: Ditto.

        * kjs/array_object.cpp: (compareWithCompareFunctionForQSort): Go back to
	doing a clear and two appends here. Fast with the new list implementation.

        * kjs/collector.h: Remove _COLLECTOR hack and just make rootObjectClasses
	return a const void *.
        * kjs/collector.cpp: Remove _COLLECTOR hack, and various other minor tweaks.

2002-11-22  Darin Adler  <darin@apple.com>

	- prepare to reimplement KJS::List; move to its own file, add statistics

        * kjs/function_object.cpp: (FunctionProtoFuncImp::call): Use new copyTail()
	function rather than copy() and removeFirst().

        * kjs/identifier.cpp: Add statistics, off by default.
        * kjs/property_map.cpp: Add statistics, off by default.

        * kjs/list.cpp: Added. Moved code here. To be rewritten.
        * kjs/list.h: Added. Moved interface here. To be rewritten.

        * kjs/types.cpp: Removed.
        * kjs/types.h: Now just an empty header that includes other headers.

        * JavaScriptCore.pbproj/project.pbxproj: Add new files, rearrange.

2002-11-22  Maciej Stachowiak  <mjs@apple.com>

	- reduce cell size to 56 bytes from 64, now that nearly all
	objects fit in that size. .5% speed gain and probably some
	footprint gain.
	
        * kjs/collector.cpp: Change CELL_SIZE from 64 to 56.

2002-11-22  Darin Adler  <darin@apple.com>

	- change ScopeChain to be a singly linked list shares tails, gives 11% gain on iBench

        * kjs/context.h:
        (ContextImp::pushScope): Make inline, use push instead of prepend, and pass imp pointer.
        (ContextImp::popScope): Make inline, use pop instead of removeFirst.
        * kjs/function.cpp: (DeclaredFunctionImp::DeclaredFunctionImp): No need to copy.
        * kjs/function_object.cpp: (FunctionObjectImp::construct): Use push instead of
	prepend, and pass imp pointer.
        * kjs/internal.cpp: (ContextImp::ContextImp): Use clear, push instead of prepend,
	and pass imp pointers.
        * kjs/nodes.cpp: (ResolveNode::evaluateReference): Use isEmpty, pop, and top instead
	of ScopeChainIterator.
        * kjs/object.h: Change _scope to be a NoRefScopeChain.
        * kjs/object.cpp: No need to initialize _scope any more, since it's not a NoRefScopeChain.

        * kjs/scope_chain.h: Rewrite, different implementation and interface.
        * kjs/scope_chain.cpp: More of the same.

2002-11-22  Maciej Stachowiak  <mjs@apple.com>

	- a simple change for .4% gain on ibench - instead of unmarking
	all objects at the start of collection, instead unmark as part of
	the sweep phase
	
        * kjs/collector.cpp:
        (Collector::collect): Remove separate unmarking pass and instead
	unmark the objects that don't get collected during the sweep
	phase.

2002-11-21  Darin Adler  <darin@apple.com>

	- stop garbage collecting the ActivationImp objects, gets 3% on iBench
	- pave the way to separate the argument lists from scope chains

        * kjs/context.h: Added. Moved ContextImp here so it can use things defined
	in function.h

        * kjs/scope_chain.h: Added. Starting as a copy of List, to be improved.
        * kjs/scope_chain.cpp: Added. Starting as a copy of List, to be improved.

        * JavaScriptCore.pbproj/project.pbxproj: Rearranged things, added context.h.

        * kjs/function.cpp:
        (FunctionImp::call): Pass InterpreterImp, not ExecState, to ContextImp.
        (DeclaredFunctionImp::DeclaredFunctionImp): List -> ScopeChain.
        (ActivationImp::createArgumentsObject): ArgumentList -> List.
        (GlobalFuncImp::call): Pass InterpreterImp, not an ExecState, to ContextImp.
        * kjs/function.h: List -> ScopeChain.
        * kjs/function_object.cpp: (FunctionObjectImp::construct): List -> ScopeChain.
        * kjs/internal.cpp:
        (ContextImp::ContextImp): Set the context in the interpreter.
        (ContextImp::~ContextImp): Set the context in the interpreter to the caller.
        (ContextImp::mark): Mark all the activation objects.
        (InterpreterImp::InterpreterImp): Initialize context to 0.
        (InterpreterImp::mark): Mark the top context.
        (InterpreterImp::evaluate): Pass InterpreterImp to ContextImp.
        * kjs/internal.h: Move ContextImp to its own header. Add setContext to InterpreterImp.
        * kjs/interpreter.cpp: (Context::scopeChain): List -> ScopeChain.
        * kjs/interpreter.h: List -> ScopeChain.
        * kjs/nodes.cpp:
        (ResolveNode::evaluateReference): List -> ScopeChain.
        (FuncDeclNode::processFuncDecl): List -> ScopeChain.
        (FuncExprNode::evaluate): List -> ScopeChain.
        * kjs/object.cpp: List -> ScopeChain.
        * kjs/object.h: List -> ScopeChain.

        * kjs/types.h: Remove needsMarking features from List.
        * kjs/types.cpp: Ditto.

2002-11-21  Maciej Stachowiak  <mjs@apple.com>

	- reduced the size of PropertyMap by storing sizes and such in the
	dynamically allocated part of the object to reduce the size of
	ObjectImp - .5% speed improvement on JS iBench.
	
        * kjs/property_map.cpp:
        * kjs/property_map.h:

2002-11-21  Maciej Stachowiak  <mjs@apple.com>

        * Makefile.am: Pass symroots for this tree to pbxbuild.

=== Alexander-33 ===

2002-11-21  Darin Adler  <darin@apple.com>

        * kjs/property_map.cpp: More assertions.

2002-11-21  Darin Adler  <darin@apple.com>

        * kjs/property_map.cpp: Turn that consistency check back off.

2002-11-21  Darin Adler  <darin@apple.com>

	- someone somewhere must be defining a macro named check, causing a compile failure in WebCore

	Rename check() to checkConsistency().

        * kjs/property_map.h: Rename.
        * kjs/property_map.cpp: Yes, rename.

2002-11-21  Darin Adler  <darin@apple.com>

	- add self-check to property map in hopes of finding the cnet.com bug

        * kjs/property_map.h: Add check() function.
        * kjs/property_map.cpp: Add the checking, controlled by DO_CONSISTENCY_CHECK.

	 - fixed UChar interface so it's not so slow in debug builds

        * kjs/ustring.h: Nothing in UChar needs to be private.

        * kjs/function.cpp: (GlobalFuncImp::call):
        * kjs/function_object.cpp: (FunctionObjectImp::construct):
        * kjs/identifier.cpp:
        * kjs/lexer.cpp: (Lexer::setCode), (Lexer::shift):
        * kjs/lookup.cpp: (keysMatch):
        * kjs/ustring.cpp: (UString::Rep::computeHash), (KJS::compare):
	Use the "uc" field instead of the "unicode()" inline function.

2002-11-21  Darin Adler  <darin@apple.com>

	- fixed a null-dereference I ran into while trying to reproduce bug 3107351

        * kjs/function.h: Change ActivationImp constructor to take context parameter.
        * kjs/function.cpp: (ActivationImp::ActivationImp): Take context parameter,
	not execution state parameter.

        * kjs/internal.cpp: (ContextImp::ContextImp): Initialize activation object
	from context, not execution state, because the new context is not yet in the
	execution state.

2002-11-20  Darin Adler  <darin@apple.com>

	- added a feature for Richard to use in his back/forward cache

        * kjs/object.h: Added save/restoreProperties.
        * kjs/property_map.h: Here too.
        * kjs/property_map.cpp: Here too.

2002-11-20  Darin Adler  <darin@apple.com>

	- created argument list objects only on demand for a 7.5% speedup

        * kjs/function.h: Change ActivationImp around.
        * kjs/function.cpp:
        (FunctionImp::call): Pass a pointer to the arguments list to avoid ref/unref.
        (FunctionImp::get): Get the function pointer from the context directly,
	not the activation object.
        (ArgumentsImp::ArgumentsImp): Add an overload that takes no arguments.
        (ActivationImp::ActivationImp): Store a context pointer and an arguments object pointer.
        (ActivationImp::get): Special case for arguments, create it and return it.
        (ActivationImp::put): Special case for arguments, can't be set.
        (ActivationImp::hasProperty): Special case for arguments, return true.
        (ActivationImp::deleteProperty): Special case for arguments, refuse to delete.
        (ActivationImp::mark): Mark the arguments object.
        (ActivationImp::createArgumentsObject): Do the work of actually creating it.
        (GlobalFuncImp::call): Use stack-based objects for the ContextImp and ExecState.

        * kjs/internal.h: Keep function and arguments pointer in the context.
        * kjs/internal.cpp:
        (ContextImp::ContextImp): Don't pass in the func and args when making an ActivationImp.
        (InterpreterImp::evaluate): Use stack-based objects here.

        * kjs/types.h: Add ArgumentList as a synonym for List, soon to be separate.

2002-11-20  Maciej Stachowiak  <mjs@apple.com>

	Reduced the size of ValueImp by 8 bytes for a .5% speedup.
	
        * kjs/value.h: Removed destructed flag. Made refcount and flag 16
	bits each.
        * kjs/value.cpp:
        (ValueImp::~ValueImp): Don't set destructed flag.

2002-11-20  Darin Adler  <darin@apple.com>

        * kjs/types.cpp: Keep ref count for the whole lists of nodes.
	Doesn't speed things up much, less than 1%.

2002-11-20  Maciej Stachowiak  <mjs@apple.com>

        * kjs/collector.cpp:
        (Collector::allocate): Clear the flags on newly allocated objects.

2002-11-20  Darin Adler  <darin@apple.com>

	- oops, checked in big regression instead of 5% speedup

        * kjs/function.cpp: (ActivationImp::ActivationImp): Make a marking
	list, not a refing list.

	- a cut at the sparse array implementation

        * kjs/array_instance.h: Keep storageLength separate from length.
        * kjs/array_object.cpp:
        (ArrayInstanceImp::ArrayInstanceImp): Start with storageLength == length.
        (ArrayInstanceImp::get): Check against storage length.
        (ArrayInstanceImp::put): Ditto.
        (ArrayInstanceImp::hasProperty): Ditto.
        (ArrayInstanceImp::deleteProperty): Ditto.
        (ArrayInstanceImp::setLength): Only enlarge storage length up to a cutoff.
        (ArrayInstanceImp::mark): Use storageLength.
        (ArrayInstanceImp::pushUndefinedObjectsToEnd): Added FIXME.

2002-11-20  Darin Adler  <darin@apple.com>

	- decrease ref/deref -- 5% speedup in iBench

        * JavaScriptCore.pbproj/project.pbxproj: Added array_instance.h
        * kjs/array_instance.h: Added so it can be shared by function.h.

        * kjs/array_object.cpp:
        * kjs/array_object.h:
        * kjs/bool_object.cpp:
        * kjs/bool_object.h:
        * kjs/collector.cpp:
        * kjs/date_object.cpp:
        * kjs/date_object.h:
        * kjs/error_object.cpp:
        * kjs/function.cpp:
        * kjs/function.h:
        * kjs/function_object.cpp:
        * kjs/internal.cpp:
        * kjs/internal.h:
        * kjs/math_object.cpp:
        * kjs/nodes.cpp:
        * kjs/number_object.cpp:
        * kjs/object.cpp:
        * kjs/object.h:
        * kjs/object_object.cpp:
        * kjs/property_map.cpp:
        * kjs/reference.cpp:
        * kjs/reference.h:
        * kjs/regexp_object.cpp:
        * kjs/string_object.cpp:
        * kjs/string_object.h:
        * kjs/value.cpp:
        * kjs/value.h:
	Switched lots of interfaces so they don't require ref/deref.

2002-11-20  Maciej Stachowiak  <mjs@apple.com>

	Fixed the two most obvious problems with the new GC for another 6%
	improvement.
	
        * kjs/collector.cpp:
        (Collector::allocate): Don't bother doing the bit tests on a bitmap word if
	all it's bits are on.
        (Collector::collect): Track memoryFull boolean.
        * kjs/collector.h: Inlined outOfMemory since it was showing up on profiles.

2002-11-20  Maciej Stachowiak  <mjs@apple.com>

	Rewrote garbage collector to make blocks of actual memory instead
	of blocks of pointers. 7% improvement on JavaScript
	iBench. There's still lots of room to tune the new GC, this is
	just my first cut.
	
        * kjs/collector.cpp:
        (Collector::allocate):
        (Collector::collect):
        (Collector::size):
        (Collector::outOfMemory):
        (Collector::finalCheck):
        (Collector::numGCNotAllowedObjects):
        (Collector::numReferencedObjects):
        (Collector::liveObjectClasses):
        * kjs/collector.h:
        * kjs/function.cpp:
        (ActivationImp::ActivationImp):
        * kjs/function.h:

2002-11-20  Darin Adler  <darin@apple.com>

	- on the road to killing ActivationImp

        * kjs/function.h: Add get/put to FunctionImp. Remove argumentsObject() from
	ActivationImp. Add function() to ActivationImp.
        * kjs/function.cpp:
        (FunctionImp::FunctionImp): No arguments property.
        (FunctionImp::call): No need to set up the arguments property.
        (FunctionImp::parameterString): Remove ** strangeness.
        (FunctionImp::processParameters): Ditto.
        (FunctionImp::get): Added, handles arguments and length properties.
        (FunctionImp::put): Ditto.
        (FunctionImp::hasProperty): Ditto.
        (FunctionImp::deleteProperty): Ditto.
        (ActivationImp::ActivationImp): Store a function pointer so we can find it
	in the context.

        * kjs/function_object.cpp: (FunctionObjectImp::construct): No need to set up
	arguments property.
        * kjs/nodes.cpp: (FuncExprNode::evaluate): No need to set up length property.

        * kjs/internal.h: Return ObjectImp * for activation object.

        * kjs/interpreter.h: Remove stray declaration of ExecStateImp.

2002-11-20  Darin Adler  <darin@apple.com>

	- add a couple of list operations to avoid clearing lists so much during sorting; gives 1.5% iBench

        * kjs/types.h: Added replaceFirst/replaceLast.
        * kjs/types.cpp: (List::replaceFirst), (List::replaceLast): Added.

        * kjs/array_object.cpp: (compareWithCompareFunctionForQSort): Use replaceFirst/replaceLast.

        * kjs/property_map.cpp: Put in an ifdef so I can re-add/remove the single entry to see if
	it has outlived its usefulness. (It hasn't yet.)

2002-11-20  Darin Adler  <darin@apple.com>

	- atomic identifiers; gives another 6.5% in the iBench suite

        * kjs/identifier.h: Did the real thing.
        * kjs/identifier.cpp: Ditto.

        * kjs/property_map.h: _tableSizeHashMask -> _tableSizeMask
        * kjs/property_map.cpp: The above, plus take advantage of comparing
	by pointer instead of by comparing bytes.

2002-11-19  Darin Adler  <darin@apple.com>

	- a few more globals for often-used property names
	- conversion to Identifier from UString must now be explicit

        * kjs/error_object.cpp:
        * kjs/function.cpp:
        * kjs/function_object.cpp:
        * kjs/identifier.cpp:
        * kjs/identifier.h:
        * kjs/lexer.cpp:
        * kjs/nodes.cpp:
        * kjs/number_object.cpp:
        * kjs/object.cpp:
        * kjs/object.h:
        * kjs/string_object.cpp:
        * kjs/testkjs.cpp:
        * kjs/ustring.cpp:
        * kjs/ustring.h:

2002-11-19  Darin Adler  <darin@apple.com>

	- another step towards atomic identifiers; storing hash in the string rep. gives about
	a 1.5% speedup in the JavaScript iBench

        * kjs/ustring.h: Add a hash field to UString::Rep.
        * kjs/ustring.cpp:
        (UString::Rep::create): Set hash to uninitialized value.
        (UString::Rep::destroy): Do the deleting in her, and call Identifier if needed.
        (UString::Rep::computeHash): Added.
        (UString::append): Set hash to 0 when modifying the string in place.
        (UString::operator=): Ditto.

        * kjs/property_map.cpp: Use the hash from UString.

        * kjs/identifier.h: Added aboutToDestroyUStringRep.
        * kjs/identifier.cpp: (Identifier::aboutToDestroyUStringRep): Added.

2002-11-19  Darin Adler  <darin@apple.com>

	- next step towards atomic identifiers; Identifier is no longer derived from UString

        * kjs/identifier.h: Remove base class and add _ustring member.
        * kjs/identifier.cpp: Add null and an == that works with const char *.
        * kjs/property_map.cpp: Get rep through _ustring.

        * kjs/function.cpp: (FunctionImp::parameterString): Call ustring().
        * kjs/function_object.cpp: (FunctionProtoFuncImp::call): Ditto.
        * kjs/nodes.cpp:
        (PropertyNode::evaluate): Ditto.
        (VarDeclNode::evaluate): Ditto.
        (ForInNode::execute): Ditto.
        * kjs/nodes2string.cpp: (SourceStream::operator<<): Add overload for Identifier.
        * kjs/reference.cpp: (Reference::getValue): Call ustring().
        * kjs/regexp_object.cpp: (RegExpObjectImp::get): Call ustring().

2002-11-19  Darin Adler  <darin@apple.com>

	- fixed memory trasher

        * kjs/ustring.cpp: (UString::from): Fix "end of buffer" computation.

2002-11-19  Darin Adler  <darin@apple.com>

	- a first step towards atomic identifiers in JavaScript

	Most places that work with identifiers now use Identifier
	instead of UString.

        * kjs/identifier.cpp: Added.
        * kjs/identifier.h: Added.
        * JavaScriptCore.pbproj/project.pbxproj: Added files.

        * kjs/array_object.cpp:
        * kjs/array_object.h:
        * kjs/completion.cpp:
        * kjs/completion.h:
        * kjs/date_object.cpp:
        * kjs/date_object.h:
        * kjs/function.cpp:
        * kjs/function.h:
        * kjs/function_object.cpp:
        * kjs/grammar.cpp:
        * kjs/grammar.cpp.h:
        * kjs/grammar.h:
        * kjs/grammar.y:
        * kjs/internal.cpp:
        * kjs/internal.h:
        * kjs/lexer.cpp:
        * kjs/lookup.cpp:
        * kjs/lookup.h:
        * kjs/math_object.cpp:
        * kjs/math_object.h:
        * kjs/nodes.cpp:
        * kjs/nodes.h:
        * kjs/number_object.cpp:
        * kjs/number_object.h:
        * kjs/object.cpp:
        * kjs/object.h:
        * kjs/property_map.cpp:
        * kjs/property_map.h:
        * kjs/reference.cpp:
        * kjs/reference.h:
        * kjs/regexp_object.cpp:
        * kjs/regexp_object.h:
        * kjs/string_object.cpp:
        * kjs/string_object.h:

2002-11-19  Darin Adler  <darin@apple.com>

	- fix hash function and key comparison for the other kind of hash table; yields 3%

        * kjs/lookup.cpp:
        (keysMatch): Added.
        (Lookup::findEntry): Don't allocate and convert to ASCII just to search.

2002-11-19  Darin Adler  <darin@apple.com>

	- another hash table fix; yields a 2% improvement on iBench JavaScript

        * kjs/property_map.cpp: A few more places where we use & instead of %.

	- some List changes that don't affect speed yet

        * kjs/types.cpp:
        (List::prependList): Tighten up a tiny bit.
        (List::copy): Use prependList.
        * kjs/types.h: Remove appendList and globalClear.

        * kjs/interpreter.cpp: (Interpreter::finalCheck): Remove List::globalClear().

2002-11-19  Darin Adler  <darin@apple.com>

	- fixed 3105026 -- REGRESSION: DHTML menus are broken all over the place

        * kjs/types.cpp: (List::prepend): Fix backwards links in new node.

2002-11-19  Darin Adler  <darin@apple.com>

	- a fix that gives another 1.5% on the iBench JavaScript test

        * kjs/ustring.cpp: (UString::from): Stop using sprintf to format integers.

2002-11-18  Darin Adler  <darin@apple.com>

	- reduced the creation of Value objects and hoisted the property map
        into Object for another gain of about 6%

        * JavaScriptCore.pbproj/project.pbxproj: Made property_map.h public.
        * kjs/array_object.cpp:
        (compareWithCompareFunctionForQSort): Don't wrap the ValueImp * in a Value
	just to add it to a list.
        (ArrayProtoFuncImp::call): Pass the globalObject directly so we don't have
	to ref/deref.
        * kjs/function.cpp:
        (FunctionImp::call): Use a reference for the global object to avoid ref/deref.
        (GlobalFuncImp::call): Ditto.
        * kjs/internal.cpp:
        (BooleanImp::toObject): Put the object directly into the list, don't create a Value.
        (StringImp::toObject): Ditto.
        (NumberImp::toObject): Ditto.
        (InterpreterImp::evaluate): Use a reference for the global object.
        * kjs/internal.h: Return a reference for the global object.
        * kjs/interpreter.cpp: (Interpreter::globalObject): Ditto.
        * kjs/interpreter.h: Ditto.
        * kjs/object.cpp: Use _prop directly in the object, not a separate pointer.
        * kjs/object.h: Ditto.
        * kjs/types.cpp: Added List methods that work directly with ValueImp.
        (List::append): Added a ValueImp version.
        (List::prepend): Ditto.
        (List::appendList): Work directly with the ValueImp's.
        (List::prependList): Ditto.
        (List::copy): Use appendList.
        (List::empty): Use a shared global List.
        * kjs/types.h: Update for above changes.

2002-11-18  Darin Adler  <darin@apple.com>

        * kjs/property_map.cpp: Oops, copyright goes to Apple, not me.
        * kjs/property_map.h: Ditto.

2002-11-18  Darin Adler  <darin@apple.com>

	- property and string improvements giving a 7% or so improvement in JavaScript iBench

        * kjs/property_map.h: Rewrite to use a hash table.
        * kjs/property_map.cpp: Ditto.

        * kjs/string_object.h:
        * kjs/string_object.cpp:
        (StringInstanceImp::StringInstanceImp): Construct a string with the right value
	instead of putting the string in later.
        (StringInstanceImp::get): Get the length from the string, not a separate property.
        (StringInstanceImp::put): Ignore attempts to set length, since we don't put it in
	the property map.
        (StringInstanceImp::hasProperty): Return true for length.
        (StringInstanceImp::deleteProperty): Return false for length.
        (StringObjectImp::construct): Call new StringInstanceImp constructor. Don't try
	to set a length property.

        * kjs/ustring.h: Make the rep deref know how to deallocate the rep.
        * kjs/ustring.cpp:
        (UString::release): Move the real work to the rep's deref, since the hash table
	now uses the rep directly.

        * kjs/object.h: Remove unused field.

2002-11-18  Maciej Stachowiak  <mjs@apple.com>

	Change List to completely avoid going through the GC
	allocator. 3.6% performance improvement on JavaScript iBench.
	
        * kjs/internal.cpp:
        (InterpreterImp::mark): Don't mark the empty list.

	For all the methods below I basically lifted the ListImp version
	up to the List method with minor tweaks.
	
        * kjs/types.cpp:
        (ListIterator::ListIterator):
        (List::List):
        (List::operator=):
        (List::~List):
        (List::mark):
        (List::append):
        (List::prepend):
        (List::appendList):
        (List::prependList):
        (List::removeFirst):
        (List::removeLast):
        (List::remove):
        (List::clear):
        (List::clearInternal):
        (List::copy):
        (List::begin):
        (List::end):
        (List::isEmpty):
        (List::size):
        (List::at):
        (List::operator[]):
        (List::empty):
        (List::erase):
        (List::refAll):
        (List::derefAll):
        (List::swap):
        (List::globalClear):
        * kjs/types.h:

2002-11-18  Maciej Stachowiak  <mjs@apple.com>

	Fixed a horrible leak introduced with my last change that
	somehow did not show up on my machine.

        * kjs/types.cpp:
        (List::List): Mark ListImp as GC allowed.

2002-11-18  Maciej Stachowiak  <mjs@apple.com>

	Another step towards the List conversion: stop inheriting from Value.
	
        * kjs/types.cpp:
        (ListIterator::ListIterator):
        (List::List):
        (List::operator=):
        (List::~List):
        (List::mark):
        (List::append):
        (List::prepend):
        (List::appendList):
        (List::prependList):
        (List::removeFirst):
        (List::removeLast):
        (List::remove):
        (List::clear):
        (List::copy):
        (List::begin):
        (List::end):
        (List::isEmpty):
        (List::size):
        (List::at):
        (List::operator[]):
        * kjs/types.h:

2002-11-18  Maciej Stachowiak  <mjs@apple.com>

	Partway to removing Value from List. Created a marking List
	variant, used it in place of ListImp.
	
        * kjs/internal.h: Removed List stuff.
        * kjs/internal.cpp:
        (InterpreterImp::mark): Call appropriate List method to do marking of
	empty ListImp.
        * kjs/object.h:
        * kjs/object.cpp: Use marking List instead of ListImp *.
        * kjs/types.h:
        * kjs/types.cpp:
        (List::List): New boolean needsMarking parameter. 
        (List::operator=): Perform trickery related to needsMarking.
        (List::~List): Likewise.
        (List::mark): Mark the ListImp.
        (List::markEmptyList):
	(ListImp::*): Moved here fron internal.cpp, they will be
	integrated into the relevant List methods soon.

2002-11-18  Darin Adler  <darin@apple.com>

	- another string constant discovered that can be optimized

        * kjs/object.h: Add a property name constant for "__proto__".
        * kjs/object.cpp: Define it.
	(ObjectImp::get): Use it.
	(ObjectImp::hasProperty): Use it.

	- prepare to turn PropertyMap into a hash table

        * kjs/object.cpp:
	(ObjectImp::mark): Use the new PropertyMap::mark().
	(ObjectImp::put): Use the new overload of PropertyMap::get().
	(ObjectImp::deleteProperty): Use the new overload of PropertyMap::get().
	(ObjectImp::propList): Use PropertyMap::addEnumerablesToReferenceList().

        * kjs/property_map.h: Remove PropertyMapNode and make all node-related methods private.
	Add mark(), a new overload of get() that returns attributes, a clear() that takes no attributes,
	and addEnumerablesToReferenceList().
        * kjs/property_map.cpp:
	(PropertyMap::get): Added new overload.
	(PropertyMap::clear): Added new overload.
	(PropertyMap::mark): Added.
	(PropertyMap::addEnumerablesToReferenceList): Added.

        * kjs/ustring.h: Added a hash function.
        * kjs/ustring.cpp: (KJS::hash): Added.

2002-11-18  Darin Adler  <darin@apple.com>

	- simplified the ExecState class, which was showing up in profiles
        
        Sped up JavaScript iBench by 6%.

        * kjs/interpreter.h: Removed the level of indirection, and made it all inline.
        * kjs/interpreter.cpp: Removed ExecState implementation from here altogether.

	- fixed an oversight in my sort speedup

        * kjs/array_object.h: Add pushUndefinedObjectsToEnd.
        * kjs/array_object.cpp:
        (ArrayInstanceImp::sort): Call pushUndefinedObjectsToEnd.
        (ArrayInstanceImp::pushUndefinedObjectsToEnd): Added.
	Pushes all undefined to the end of the array.

2002-11-18  Darin Adler  <darin@apple.com>

	- fix worst speed problems on the sort page of the iBench JavaScript test

	Sped up JavaScript iBench by 70%, the sort page by 88%.

        * kjs/array_object.h: Add array-specific sort functions.
        * kjs/array_object.cpp:
        (compareByStringForQSort): Added.
        (ArrayInstanceImp::sort): Added.
        (compareWithCompareFunctionForQSort): Added.
        (ArrayProtoFuncImp::call): Use ArrayInstanceImp::sort if the object being
	sorted is actually an array.

        * kjs/object.h: Add argumentsPropertyName.
        * kjs/object.cpp: Add argumentsPropertyName.
        * kjs/function.cpp:
        (FunctionImp::FunctionImp): Use argumentsPropertyName to avoid making a UString.
        (FunctionImp::call): Ditto.
        (ActivationImp::ActivationImp): Ditto.
        * kjs/function_object.cpp: (FunctionObjectImp::construct): Ditto.

        * kjs/ustring.h: Added compare function for -1/0/+1 comparison.
        * kjs/ustring.cpp: (KJS::compare): Added.

2002-11-18  Maciej Stachowiak  <mjs@apple.com>

	Change ArgumentListNode operations to be iterative instead of
	recursive. This probably fixes 3095446 (Crash in
	KJS::ArgumentListNode::ref()) but I can't reproduce it myself so
	I'm not 100% sure. I think the original bug was a stack overflow
	and this change would remove that possibility.
	
        * kjs/nodes.cpp:
        (ArgumentListNode::ref): Make iterative.
        (ArgumentListNode::deref): Make iterative.
        (ArgumentListNode::evaluateList): Make iterative.

=== Alexander-32 ===

2002-11-14  Darin Adler  <darin@apple.com>

	- fixed 3101243 -- excite passes date that can't be parsed, results in bogus date at top right corner

        * kjs/date_object.cpp: (KJS::KRFCDate_parseDate): Handle errors from strtol
	by checking errno. Check the "string in a haystack" to be sure it's a multiple
	of 3. Add case that allows year to be after time.

2002-11-14  Darin Adler  <darin@apple.com>

	- fixed 3101191 -- REGRESSION: Hang loading excite.com

        * kjs/date_object.cpp:
        (mktimeUsingCF): Pick an arbitrary cutoff of 3000, and return -1 if the
	year passed in is that big so we don't infinite loop. Also validate the
	rest of the date with CFGregorianDateIsValid. 
        (DateProtoFuncImp::call): Handle a -1 result from mktime.
        (DateObjectImp::construct): Check for NaN before calling mktime, and also
	handle a -1 result from mktime.
        (DateObjectFuncImp::call): Check for NaN before calling mktime, and also
	handle a -1 result from mktime.

2002-11-13  Darin Adler  <darin@apple.com>

	- fixed 3099930 -- dates/times without time zones are parsed as UTC by kjs,
	local time by other browsers

        * kjs/date_object.cpp:
        (DateProtoFuncImp::call): Handle the NaN case better, like Mozilla and OmniWeb.
        (DateObjectFuncImp::call): Return NaN rather than Undefined() for bad dates.
        (KJS::parseDate): Return NaN rather than Undefined() or 0 for bad dates.
        (KJS::KRFCDate_parseDate): Return -1 rather than 0 for bad dates.
	Assume local time if no time zone is passed. Don't return 1 if we parse 0.

2002-11-13  Darin Adler  <darin@apple.com>

        - fixed 3073230 -- JavaScript time calls do I/O by lstat()ing /etc/localtime

        * kjs/date_object.cpp:
        (formatDate): Added.
        (formatTime): Added.
        (formatLocaleDate): Added.
        (formatLocaleTime): Added.
        (DateProtoFuncImp::call): Changed to use the above functions instead of
	using strftime.

2002-11-08  Darin Adler  <darin@apple.com>

        * kjs/date_object.cpp:
        (ctimeUsingCF): Added.
        (timeUsingCF): Added.

2002-11-07  Darin Adler  <darin@apple.com>

        * kjs/date_object.cpp: (mktimeUsingCF): Fix storage leak.

2002-11-07  Maciej Stachowiak  <mjs@apple.com>

	- partial fix to 3073230 - JavaScript time calls do I/O by
	lastat()ing /etc/localtime
	
        * kjs/date_object.cpp:
        (mktimeUsingCF): Implementation of mktime using CF.

=== Alexander-31 ===

2002-11-01  Darin Adler  <darin@apple.com>

        * kjs/object.cpp: Make the same change Maciej just did, but to the
	other constructor right next to the one he changed.

2002-10-31  Maciej Stachowiak  <mjs@apple.com>

	- fixed 3082660 - REGRESSION: one ListImp leaks opening/closing nearly empty web page
	
        * kjs/object.cpp: Set gc allowed on freshly created ListImp, since
	there is no List wrapper for it.

2002-10-31  Darin Adler  <darin@apple.com>

        * kjs/grammar.y: Fix the APPLE_CHANGES thing here too.
        * kjs/grammar.cpp: Regenerated this file.

=== Alexander-30 ===

2002-10-30  Darin Adler  <darin@apple.com>

	- fixed 3073230 -- Alex is doing file I/O when executing JavaScript by asking for localtime

	I fixed this by using Core Foundation time functions instead.

        * kjs/date_object.cpp:
        (tmUsingCF): Function that uses Core Foundation to get the time and then puts it into
	a tm struct.
        (gmtimeUsingCF): Function used instead of gmtime (used a macro to make the substitution).
        (localtimeUsingCF): Function used instead of localtime (used a macro to make the substitution).

2002-10-26  Darin Adler  <darin@apple.com>

	- changed to use #if APPLE_CHANGES and #if !APPLE_CHANGES consistently

	We no longer do #ifdef APPLE_CHANGES or #ifndef APPLE_CHANGES.

        * kjs/collector.cpp:
        * kjs/collector.h:
        * kjs/grammar.cpp:
        * kjs/internal.cpp:
        * kjs/ustring.h:

2002-10-25  Darin Adler  <darin@apple.com>

	- fixed 3038011 -- drop-down menu hierarchy broken at yahoo new acct page

        * kjs/array_object.cpp: (ArrayProtoFuncImp::call):
	Fix bug calling concat on an empty array. The old code tried to
	optimize in a way that would prevent appending any arrays until
	at least one element was in the destination array. So if you were
	concatenating a non-empty array into an empty array, you got an empty array.

=== Alexander-29 ===

=== Alexander-28 ===

2002-10-10  Darin Adler  <darin@apple.com>

	- fixed 3072643 -- infinite loop in JavaScript code at walgreens.com

	The problem is that "xxx".indexOf("", 1) needs to return 1, but we
	were returning 0.

        * kjs/ustring.cpp:
        (UString::find): Return pos, not 0, when the search string is empty.
        (UString::rfind): Make sure that pos is not past the end of the string,
	taking into account the search string; fixes a potential read off the end
	of the buffer. Also return pos, not 0, when the search string is empty.

=== Alexander-27 ===

2002-10-07  Darin Adler  <darin@apple.com>

	Fixed absurdly high memory usage when looking at pages that use a lot of JavaScript.

        * kjs/collector.cpp:
        (Collector::allocate): Implement a new policy of doing a garbage collect every 1000
	allocations. The old policy was both complicated and misguided.
        (Collector::collect): Zero out the "number of allocations since last collect".

2002-10-06  Darin Adler  <darin@apple.com>

	I noticed some broken lists at mapblast.com and tracked it down to this.

        * kjs/array_object.cpp:
        (ArrayInstanceImp::put): Don't truncate the list; only extend the length if
	it's not already long enough.
        (ArrayProtoFuncImp::call): Fix some ifdef'd code so it compiles if you turn
	the ifdefs on.

2002-10-04  Darin Adler  <darin@apple.com>

        Fixed problems parsing numbers that are larger than a long with parseInt.

        * kjs/config.h: Define HAVE_FUNC_STRTOLL.
        * kjs/function.cpp: (GlobalFuncImp::call):
	Change parseInt to use strtoll if available.

=== Alexander-26 ===

2002-09-27  Darin Adler  <darin@apple.com>

	- fixed 3033969 -- repro crash (infinite recursion in JavaScript)
	clicking on "screens" option at fsv.sf.net

        * kjs/object.h: Change recursion limit to 100 levels rather than 1000.

=== Alexander-25 ===

2002-09-26  Darin Adler  <darin@apple.com>

	Fix the infinity problem Dave worked around. We didn't have the
	configuration flags set right to make infinity work. Setting those
	properly made everything work without changes to min and max.

        * kjs/config.h: Define HAVE_FUNC_ISINF, HAVE_STRING_H, and
	also WORDS_BIGENDIAN (if on ppc).

        * kjs/math_object.cpp: (MathFuncImp::call): Roll out min and max
	changes from yesterday.

2002-09-25  David Hyatt  <hyatt@apple.com>

	Fix the impls of min/max to not use +inf/-inf when you have
	arguments.  Technically there's still a bug here for the no
	argument case, probably caused by a screwup when +inf/-inf are
	converted to doubles.
	
        * kjs/math_object.cpp:
        (MathFuncImp::call):

2002-09-25  Darin Adler  <darin@apple.com>

	- fixed 3057964 -- JS problem performing MD5 script embedded in yahoo login page

        * kjs/simple_number.h: Fix incorrect check for sign bit that was munging numbers
	in the range 0x10000000 to 0x1FFFFFFF.

=== Alexander-24 ===

=== Alexander-22 ===

2002-09-05  Maciej Stachowiak  <mjs@apple.com>

	First baby step towards moving List away from garbage collection.
	
        * kjs/types.h: Add needsMarking boolean and make List inherit from
	Value privately instead of publicly.

2002-08-30  Darin Adler  <darin@apple.com>

        * JavaScriptCore.pbproj/project.pbxproj: Allowed the new Project Builder to put in
	encodings for each file.

=== Alexander-21 ===

=== Alexander-20 ===

2002-08-20  Darin Adler  <darin@apple.com>

	Three small changes to things that showed up in the sample.

	5% speed increase on cvs-js-performance test.
	
        * kjs/simple_number.h: Check if double is an integer with d == (double)(int)d
	instead of remainder(d, 1) == 0, saving a function call each time.

        * kjs/ustring.cpp:
        (UString::find): Compare the first character before calling memcmp for the rest.
        (UString::rfind): Ditto.
        (KJS::operator==): Don't do a strlen before starting to compare the characters.

2002-08-20  Maciej Stachowiak  <mjs@apple.com>

        * kjs/object.cpp: Don't reference other ValueImps in the
	destructor, they may have already been destroyed, and will have
	GC_ALLOWED set already in any case.

2002-08-19  Maciej Stachowiak  <mjs@apple.com>

	Fixed the bug that made sony.com menus come out wrong and made
	aa.com crash (Radar 3027762).
	
	Mode most methods inline.
	
        * kjs/completion.cpp:
        * kjs/completion.h:

2002-08-19  Maciej Stachowiak  <mjs@apple.com>

	Maintain stack of old "arguments" property values for functions
	implicitly on the system stack instead of explicitly in the
	FunctionImp. This eliminates only a trivial number of GC
	allocations (less than 200) but eliminates one of the two cases
	where a ListImp * is stored directly, paving the way to separate
	List from Value.
	
        * kjs/function.h: Remove argStack, pushArgs and popArgs.
        * kjs/function.cpp:
        (FunctionImp::FunctionImp): Don't initalize argStack.
        (FunctionImp::~FunctionImp): Remove comment about argStack.
        (FunctionImp::mark): Don't mark the argStack.
        (FunctionImp::call): Save old "arguments" property in a Value,
	where it will be GC-protected, rather than keeping a list, and
	restore the old value when done executing.

2002-08-18  Darin Adler  <darin@apple.com>

        * kjs/internal.cpp: (KJS::printInfo): Remove one more CompletionType
	that Maciej missed.

2002-08-18  Maciej Stachowiak  <mjs@apple.com>

	Remove stray references to CompletionType and CompletionImp.
	
        * kjs/completion.h:
        * kjs/object.cpp:
        * kjs/value.h:

2002-08-18  Maciej Stachowiak  <mjs@apple.com>

	Separated Completion from Value and made it a pure stack
	object. This removed another 160,000 of the remaining 580,000
	garbage collected object allocations.

	6% speed increase on cvs-js-performance test.
	
        * kjs/completion.cpp: Added. New implementation that doesn't
	require a ValueImp *.
        (Completion::Completion):
        (Completion::complType):
        (Completion::value):
        (Completion::target):
        (Completion::isValueCompletion):
        * kjs/completion.h: Added.
        * kjs/function.cpp:
	(GlobalFuncImp::call): Removed some (apparently mistaken) uses of
	Completion as a Value.
        * kjs/internal.cpp:
        * kjs/internal.h:
        * kjs/types.cpp: Removed Completion stuff.
        * kjs/types.h: Removed Completion stuff.
        * JavaScriptCore.pbproj/project.pbxproj: Added new header.

2002-08-16  Darin Adler  <darin@apple.com>

	Fix the Development build.

        * kjs/object.cpp: Take out a use of ReferenceType.

        * kjs/ustring.h: Added a bit more inlining.
        * kjs/ustring.cpp: Moved the function out of here.

2002-08-16  Maciej Stachowiak  <mjs@apple.com>

	Final step of the Reference change. Completely separate Reference
	from Value, and eliminate ReferenceImp.

	18% speedup on cvs-js-performance test.

        * kjs/internal.cpp, kjs/internal.h: Remove ReferenceImp.
        * kjs/nodes.cpp:
        (Node::evaluateReference): Use Reference::makeValueReference(),
	not ConstReference.
        * kjs/reference.cpp:
        (Reference::Reference): New implementation, handles both regular
	and value references.
        (Reference::makeValueReference): Incorporate functionality of ConstReference
	into this class.
        (Reference::getBase): New implementation (incorporates error vase
	for value references).
	(Reference::getPropertyName): New implementation (incorporates error case
	for value references).
        (Reference::putValue): New implementation (incorporates error case
	for value references).
        (Reference::deleteValue): New implementation (incorporates error case
	for value references).
        (Reference::getValue): New implementation (incorporates special case
	for value references).
        (Reference::isMutable): New implementation.
	* kjs/reference.h: New implementation that merges ReferenceImp
	into the stack object.
        * kjs/value.h, kjs/value.cpp: Removed all reference-related method.

2002-08-16  Darin Adler  <darin@apple.com>

	- fixed 3026184 -- Hang going to http://aa.com/ while executing JavaScript

        * kjs/simple_number.h: (SimpleNumber::value): Fixed conversion to a negative
	number. The technique of using division was no good. Instead, or in the sign
	bits as needed.

2002-08-16  Maciej Stachowiak  <mjs@apple.com>

        * kjs/reference_list.h: Must include headers with "", not
	<>. D'oh!

2002-08-16  Maciej Stachowiak  <mjs@apple.com>

        * JavaScriptCore.pbproj/project.pbxproj: Install reference.h and
	reference_list.h so WebCore compiles (duh).

2002-08-16  Maciej Stachowiak  <mjs@apple.com>

        * JavaScriptCore.pbproj/project.pbxproj:
        * kjs/internal.cpp:
        * kjs/internal.h:
        * kjs/nodes.cpp:
        (Node::evaluateReference):
        * kjs/reference.cpp:
        (Reference::Reference):
        (Reference::makeValueReference):
        (Reference::getBase):
        (Reference::getPropertyName):
        (Reference::getValue):
        (Reference::putValue):
        (Reference::deleteValue):
        (Reference::isMutable):
        * kjs/reference.h:
        * kjs/reference_list.h:
        * kjs/value.cpp:
        (ValueImp::dispatchToUInt32):
        * kjs/value.h:

2002-08-16  Maciej Stachowiak  <mjs@apple.com>

	Next step: reimplement ReferenceList from scratch, and store it as
	an actual Reference object, so ReferenceList no longer depends on
	Reference being a Value or having a ReferenceImp. A resizing
	vector might be even better the way this is used.

	Also moved Reference to its own header and implementation file in
	preparation for reimplementing it.
	
        * JavaScriptCore.pbproj/project.pbxproj:
        * kjs/nodes.cpp:
        (ForInNode::execute):
        * kjs/reference.cpp: Added.
        (Reference::Reference):
        (Reference::dynamicCast):
        (ConstReference::ConstReference):
        * kjs/reference.h: Added.
        * kjs/reference_list.cpp: Added.
        (ReferenceList::ReferenceList):
        (ReferenceList::operator=):
        (ReferenceList::swap):
        (ReferenceList::append):
        (ReferenceList::~ReferenceList):
        (ReferenceList::begin):
        (ReferenceList::end):
        (ReferenceListIterator::ReferenceListIterator):
        (ReferenceListIterator::operator!=):
        (ReferenceListIterator::operator->):
        (ReferenceListIterator::operator++):
        * kjs/reference_list.h:
        * kjs/types.cpp:
        * kjs/types.h:

2002-08-16  Maciej Stachowiak  <mjs@apple.com>

	Fix Development build - some NDEBUG code had to be changed for the
	Value/Reference split.
	
        * kjs/internal.cpp:
        (KJS::printInfo):
        * kjs/nodes.cpp:
        (FunctionCallNode::evaluate):

2002-08-16  Maciej Stachowiak  <mjs@apple.com>

        * kjs/reference_list.h: Added file I forgot to check in last time.

2002-08-15  Maciej Stachowiak  <mjs@apple.com>

	Phase 1 of optimization to stop allocating references through the
	collector. This step clearly splits evaluating to a reference and
	evaluating to a value, and moves all of the reference-specific
	operations from Value to Reference. A special ConstReference class
	helps out for the one case where you need special reference
	operations if the result is a reference, and not otherwise.

	Also, Reference now inherits privately from Value, and there is a
	new ReferenceList class that inherits privately from List, so the
	uses of Reference and Value are now completely orthogonal. This
	means that as the next step, their implementations can be
	completely disentangled.
	
	This step has no actual performance impact.
	
        * kjs/collector.cpp:
        (Collector::collect):
        * kjs/nodes.cpp:
        (Node::evaluateReference):
        (ResolveNode::evaluate):
        (ResolveNode::evaluateReference):
        (ElementNode::evaluate):
        (PropertyValueNode::evaluate):
        (AccessorNode1::evaluate):
        (AccessorNode1::evaluateReference):
        (AccessorNode2::evaluate):
        (AccessorNode2::evaluateReference):
        (ArgumentListNode::evaluateList):
        (NewExprNode::evaluate):
        (FunctionCallNode::evaluate):
        (PostfixNode::evaluate):
        (DeleteNode::evaluate):
        (VoidNode::evaluate):
        (TypeOfNode::evaluate):
        (PrefixNode::evaluate):
        (UnaryPlusNode::evaluate):
        (NegateNode::evaluate):
        (BitwiseNotNode::evaluate):
        (LogicalNotNode::evaluate):
        (MultNode::evaluate):
        (AddNode::evaluate):
        (ShiftNode::evaluate):
        (RelationalNode::evaluate):
        (EqualNode::evaluate):
        (BitOperNode::evaluate):
        (BinaryLogicalNode::evaluate):
        (ConditionalNode::evaluate):
        (AssignNode::evaluate):
        (CommaNode::evaluate):
        (VarDeclNode::evaluate):
        (ExprStatementNode::execute):
        (IfNode::execute):
        (DoWhileNode::execute):
        (WhileNode::execute):
        (ForNode::execute):
        (ForInNode::execute):
        (ReturnNode::execute):
        (WithNode::execute):
        (CaseClauseNode::evaluate):
        (SwitchNode::execute):
        (ThrowNode::execute):
        * kjs/nodes.h:
        * kjs/types.cpp:
        (ConstReference::ConstReference):
        * kjs/types.h:
        * kjs/value.h:

2002-08-15  Darin Adler  <darin@apple.com>

	Tweaks and small bug fixes to Maciej's excellent new fixnum optimization.
	Also updated or removed comments that call it "fixnum" instead of "simple number".

        * kjs/simple_number.h: Change constant names so they don't SHOUT the way macro
	names do. Added constants for shift, min, and max. Fixed off-by-1 error that
	prevented us from using the extreme values on either end. Base the range of
	numbers on a fixed 32 bits constant rather than the size of a long, because
	code elsewhere depends on positive numbers fitting into both "unsigned" and
	"UInt32" while assuming it doesn't need to check; we can easily change this
	later. Used int types rather than long for essentially the same reason.
	Fixed the value-extraction function so it will work for negative numbers even
        if the shift is logical, not arithmetic, by using division instead.
	Renamed functions to be quite terse since they are inside a class.

        * kjs/value.h:
        * kjs/value.cpp:
        (ValueImp::dispatchToObject): Call NumberImp::toObject in a "non-virtual"
	way rather than repeating the code here.
        (ValueImp::dispatchToUInt32): Handle the negative number case correctly.
        (ValueImp::dispatchGetBase): Call ValueImp::getBase in a "non-virtual"
	way rather than repeating the code here.
        (ValueImp::dispatchGetPropertyName): Call ValueImp::getPropertyName in a
	"non-virtual" way rather than repeating the code here.
        (ValueImp::dispatchPutValue): Call ValueImp::putValue in a "non-virtual"
	way rather than repeating the code here.
        (ValueImp::dispatchDeleteValue): Call ValueImp::deleteValue in a "non-virtual"
	way rather than repeating the code here.
        (Number::Number): Fixed a bug where the double-based constructor was casting
	to long, so wouldn't do the "remainder" check.

=== Alexander-19 ===

=== Alexander-18 ===

2002-08-15  Maciej Stachowiak  <mjs@apple.com>

	Phase 2 of fixnum optimization. Store any integral number that
	will fit in two bits less than a long inside the ValueImp *
	itself, thus avoiding the need to deal with the garbage collector
	at all for these types. Such numbers comprised .5 million of the
	1.7 million ValueImps created during the cvs-js-performance test,
	so traffic through the garbage collector should be

	20% improvement on cvs-js-performance. This may also show up on
	cvs-base, but I did not compare and I am too lazy to make clean in
	WebCore yet again. 

	This also significantly reduces memory footprint on
	JavaScript-heavy pages. Size after going through
	cvs-js-performance suite is now 22MB to 17.5MB.
	
        * JavaScriptCore.pbproj/project.pbxproj:
        * kjs/simple_number.h: Added. Some inline static methods for handling
	simple numbers that are stored in the pointer.
        * kjs/ustring.h:
        * kjs/ustring.cpp:
        (UString::from): Added new overload for long.
        * kjs/value.cpp:
        (ValueImp::marked): Add special case for simple numbers.
        (ValueImp::setGcAllowed): Likewise.
	(ValueImp::toInteger): Call dispatch version of
	toUInt32(unsigned&), not the real method.
        (ValueImp::toInt32): Likewise.
        (ValueImp::toUInt32): Likewise.
        (ValueImp::toUInt16): Likewise.
        (ValueImp::dispatchType): Add special case for simple numbers.
        (ValueImp::dispatchToPrimitive): Likewise.
        (ValueImp::dispatchToBoolean): Likewise.
        (ValueImp::dispatchToNumber): Likewise.
        (ValueImp::dispatchToString): Likewise.
        (ValueImp::dispatchToObject): Likewise.
        (ValueImp::dispatchToUInt32): Likewise.
        (ValueImp::dispatchGetBase): Likewise.
        (ValueImp::dispatchGetPropertyName): Likewise.
        (ValueImp::dispatchPutValue): Likewise.
        (ValueImp::dispatchDeleteValue): Likewise.
        (Number::Number): Create a simple number instead of a full-blown
	ValueImp when possible.
        (Number::value): Likewise.
        * kjs/value.h:

2002-08-15  Maciej Stachowiak  <mjs@apple.com>

	Phase one of the "fixnum" optimization (storing small enough
	integers in the pointer). This just paves the way for the change
	by making all the virtual functions of ValueImp private and adding
	non-virtual dispatchers which can call the virtual function or
	handle fixnums specially.

	Also, I marked every place that should need a special case with a
	FIXNUM comment.
	
        * kjs/bool_object.cpp:
        (BooleanObjectImp::construct): Call dispatch method not the real method.
        * kjs/internal.h: Make toUInt32 private to make sure no one calls it directly
	on a NumberImp*.
        * kjs/nodes.cpp:
        (ForInNode::execute): Call dispatch method not the real method.
        * kjs/object.cpp:
	(ObjectImp::propList): Call dispatch method not the real method.
        * kjs/object.h:
        * kjs/string_object.cpp:
        (StringProtoFuncImp::call): Call dispatch method not the real method.
        (StringObjectImp::construct): Call dispatch method not the real method.
        * kjs/value.h:
        * kjs/value.cpp:
        (ValueImp::marked): Put a comment about required FIXNUM change.
        (ValueImp::setGcAllowed): Likewise.
        (ValueImp::dispatchType): Just call the virtual method for now.
        (ValueImp::dispatchToPrimitive): Likewise.
        (ValueImp::dispatchToBoolean): Likewise.
        (ValueImp::dispatchToNumber): Likewise.
        (ValueImp::dispatchToString): Likewise.
        (ValueImp::dispatchToObject): Likewise.
        (ValueImp::dispatchToUInt32): Likewise.
        (ValueImp::dispatchGetBase): Likewise.
        (ValueImp::dispatchGetPropertyName): Likewise.
        (ValueImp::dispatchGetValue): Likewise.
        (ValueImp::dispatchPutValue): Likewise.
        (ValueImp::dispatchDeleteValue): Likewise.

2002-08-14  Darin Adler  <darin@apple.com>

	Another pass of tweaks, including one bug fix.

        * kjs/array_object.cpp:
        (ArrayInstanceImp::ArrayInstanceImp): Use malloc, not new.
        (ArrayInstanceImp::get): Use a local variable so we don't rely on the optimizer
	to avoid indexing twice.
        (ArrayInstanceImp::hasProperty): Use a local variable, and also check against
	UndefinedImp::staticUndefined rather than doing type() != UndefinedType.

2002-08-14  Maciej Stachowiak  <mjs@apple.com>

        Simplified array handling by using NULL to represent empty cells
	instead of the Undefined object, so we can use calloc, realloc and
	memset instead of loops. Inspired by a suggestion of Darin's.

	* kjs/array_object.cpp:
        (ArrayInstanceImp::ArrayInstanceImp):
        (ArrayInstanceImp::~ArrayInstanceImp):
        (ArrayInstanceImp::get):
        (ArrayInstanceImp::hasProperty):
        (ArrayInstanceImp::deleteProperty):
        (ArrayInstanceImp::setLength):
        (ArrayInstanceImp::mark):

2002-08-14  Maciej Stachowiak  <mjs@apple.com>

        Fix major JavaScript memory leak. run-plt says cvs-base improved
	by 2% and cvs-js-performance improved by 7%. However, this was
	within the possible noise level in each case.
        
	The fix was to store ValueImp *'s in the array instead of Value
	objects, since the Value wrapper will keep a ref and make the
	object immortal.

	* kjs/array_object.cpp:
        (ArrayInstanceImp::ArrayInstanceImp):
        (ArrayInstanceImp::get):
        (ArrayInstanceImp::put):
        (ArrayInstanceImp::hasProperty):
        (ArrayInstanceImp::deleteProperty):
        (ArrayInstanceImp::setLength):
        (ArrayInstanceImp::mark):
        * kjs/array_object.h:

2002-08-13  Maciej Stachowiak  <mjs@apple.com>

	Add the ability to determine the classes of live JavaScript
	objects, to help with leak fixing.

        * kjs/collector.h, kjs/collector.cpp:
        (Collector::liveObjectClasses):

2002-08-13  Maciej Stachowiak  <mjs@apple.com>

	Small speed improvement. 3% faster on cvs-js-performance, no
	measurable change on cvs-static-urls.
	
        * kjs/collector.cpp:
        (Collector::collect): Combine 3 loops over all objects into one,
	to reduce flat time and improve locality of reference.

2002-08-12  Darin Adler  <darin@apple.com>

	Speed improvements. 19% faster on cvs-js-performance, 1% on cvs-static-urls.

	Use global string objects for length and other common property names rather
	than constantly making and destroying them. Use integer versions of get() and
	other related calls rather than always making a string.

	Also get rid of many unneeded constructors, destructors, copy constructors, and
	assignment operators. And make some functions non-virtual.

        * kjs/internal.h:
        * kjs/internal.cpp:
        (NumberImp::toUInt32): Implement.
        (ReferenceImp::ReferenceImp): Special case for numeric property names.
        (ReferenceImp::getPropertyName): Moved guts here from ValueImp. Handle numeric case.
        (ReferenceImp::getValue): Moved guts here from ValueImp. Handle numeric case.
        (ReferenceImp::putValue): Moved guts here from ValueImp. Handle numeric case.
        (ReferenceImp::deleteValue): Added. Handle numeric case.

        * kjs/array_object.h:
        * kjs/array_object.cpp: All-new array implementation that stores the elements
	in a C++ array rather than in a property map.
        (ArrayInstanceImp::ArrayInstanceImp): Allocate the C++ array.
        (ArrayInstanceImp::~ArrayInstanceImp): Delete the C++ array.
        (ArrayInstanceImp::get): Implement both the old version and the new overload that
	takes an unsigned index for speed.
        (ArrayInstanceImp::put): Implement both the old version and the new overload that
	takes an unsigned index for speed.
        (ArrayInstanceImp::hasProperty): Implement both the old version and the new overload that
	takes an unsigned index for speed.
        (ArrayInstanceImp::deleteProperty): Implement both the old version and the new overload that
	takes an unsigned index for speed.
        (ArrayInstanceImp::setLength): Added. Used by the above to resize the array.
        (ArrayInstanceImp::mark): Mark the elements of the array too.
        (ArrayPrototypeImp::ArrayPrototypeImp): Pass the length to the array instance constructor.

        * kjs/bool_object.cpp:
        * kjs/date_object.cpp:
        * kjs/error_object.cpp:
        * kjs/function.cpp:
        * kjs/function_object.cpp:
        * kjs/math_object.cpp:
        * kjs/nodes.cpp:
        * kjs/nodes.h:
        * kjs/number_object.cpp:
        * kjs/object_object.cpp:
        * kjs/regexp_object.cpp:
        * kjs/string_object.cpp:

        * kjs/nodes2string.cpp: (SourceStream::operator<<): Add a special case for char now that
	you can't create a UString from a char implicitly.

        * kjs/object.h:
        * kjs/object.cpp:
        (ObjectImp::get): Call through to the string version if the numeric version is not implemented.
        (ObjectImp::put): Call through to the string version if the numeric version is not implemented.
        (ObjectImp::hasProperty): Call through to the string version if the numeric version is not implemented.
        (ObjectImp::deleteProperty): Call through to the string version if the numeric version is not implemented.

        * kjs/types.h:
        * kjs/types.cpp:
        (Reference::Reference): Added constructors for the numeric property name case.

        * kjs/ustring.h: Made the constructor that turns a character into a string be explicit so we
	don't get numbers that turn themselves into strings.
        * kjs/ustring.cpp:
        (UString::UString): Detect the empty string case, and use a shared empty string.
        (UString::find): Add an overload for single character finds.
        (UString::rfind): Add an overload for single character finds.
        (KJS::operator==): Fix bug where it would call strlen(0) if the first string was not null.
	Also handle non-ASCII characters consistently with the rest of the code by casting to unsigned char
	just in case.

        * kjs/value.h: Make ValueImp and all subclasses non-copyable and non-assignable.
        * kjs/value.cpp:
        (ValueImp::toUInt32): New interface, mainly useful so we can detect array indices and not turn
	them into strings and back.
        (ValueImp::toInteger): Use the new toUInt32. Probably can use more improvement.
        (ValueImp::toInt32): Use the new toUInt32. Probably can use more improvement.
        (ValueImp::toUInt16): Use the new toUInt32. Probably can use more improvement.
        (ValueImp::getBase): Remove handling of the Reference case. That's in ReferenceImp now.
        (ValueImp::getPropertyName): Remove handling of the Reference case. That's in ReferenceImp now.
        (ValueImp::getValue): Remove handling of the Reference case. That's in ReferenceImp now.
        (ValueImp::putValue): Remove handling of the Reference case. That's in ReferenceImp now.
        (ValueImp::deleteValue): Added. Used so we can do delete the same way we do put.

=== Alexander-17 ===

2002-08-09  Darin Adler  <darin@apple.com>

	Some string speedups. Makes sony.com cached 11% faster on Development, but
        the improvement for Deployment should be greater.

        * kjs/ustring.h: Made it possible for UChar objects to be uninitialized, which
	gives a speed boost. Inlined CString's +=, UString's destructor, +=, and +.
        * kjs/ustring.cpp:
        (UString::UString): Optimize const char * version, which showed up
	heavily in performance analysis. Added new two-UString version, which
	makes the + operator fast. 
        (UString::ascii): Remove thread safety changes. Change static buffer to remember
	its size, and to always be at least 4096 bytes long; that way we never have to
	reallocate unless it's for a long string. Also make code to extract the characters
	significantly faster by getting rid of two pointer dereferences per character.
        (UString::is8Bit): Avoid one pointer dereference per character.
        (UString::toDouble): Use ascii() instead of cstring() to avoid copying the string.

        * kjs/collector.cpp: Remove unneeded APPLE_CHANGES.
        * kjs/regexp.cpp: Remove ifdefs around some APPLE_CHANGES that we
	want to keep, because they just fix warnings.
        * kjs/value.h: Remove obsolete APPLE_CHANGES comment.

        * JavaScriptCore.pbproj/project.pbxproj: Project Builder decided
	to move a line around in the file.

2002-08-09  Maciej Stachowiak  <mjs@apple.com>

	Fix my last change to actually call the versions of the lock functions
	that are recursive and initialize as needed.
	
        * kjs/internal.cpp:
        (InterpreterImp::InterpreterImp):
        (InterpreterImp::clear):
        (InterpreterImp::evaluate):

2002-08-09  Maciej Stachowiak  <mjs@apple.com>

        - fixed 2948835 - JavaScriptCore locking is too fine grained, makes it too slow

	* kjs/collector.cpp:
        (Collector::allocate):
        (Collector::collect):
        (Collector::finalCheck):
        (Collector::numInterpreters):
        (Collector::numGCNotAllowedObjects):
        (Collector::numReferencedObjects):
        * kjs/collector.h:
        * kjs/internal.cpp:
        (initializeInterpreterLock):
        (lockInterpreter):
        (unlockInterpreter):
        (Parser::parse):
        (InterpreterImp::InterpreterImp):
        (InterpreterImp::clear):
        (InterpreterImp::evaluate):
        * kjs/value.cpp:
        (ValueImp::ValueImp):
        (ValueImp::setGcAllowed):

=== milestone 0.5 ===

=== Alexander-16 ===

2002-08-05  Maciej Stachowiak  <mjs@apple.com>

	- fixed 3007072 - need to be able to build fat
	
        * JavaScriptCore.pbproj/project.pbxproj: Fixed DeploymentFat build.

=== Alexander-15 ===

2002-07-25  Darin Adler  <darin@apple.com>

        * JavaScriptCore.pbproj/project.pbxproj: Add DeploymentFat build style.

=== Alexander-14 ===

2002-07-21  Darin Adler  <darin@apple.com>

        * kjs/*: Roll KDE 3.0.2 changes in. Also switch to not using APPLE_CHANGES
	for some of the changes that we definitely want to contribute upstream.

2002-07-21  Maciej Stachowiak  <mjs@apple.com>

        * Makefile.am: Remove products from symroots on `make clean'.

=== Alexander-13 ===

2002-07-13  Darin Adler  <darin@apple.com>

        * Makefile.am: Don't use embed.am any more.
        * JavaScriptCore.pbproj/project.pbxproj: Use embed-into-alex instead
	of make embed.

2002-07-12  Darin Adler  <darin@apple.com>

        * kjs/ustring.h: Since <sys/types.h> includes ushort and uint now, had
	to change the includes here to be compatible with that.

2002-07-11  Darin Adler  <darin@apple.com>

        * JavaScriptCore.pbproj/project.pbxproj: To make the build of
	WebCore work without using -I to peek at JavaScriptCore sources,
	made all the Public sources Private so they are all in one directory.
	Also, made lookup.h be Private.

=== Alexander-11 ===

=== Alexander-10 ===

2002-06-25  Darin Adler  <darin@apple.com>

        * JavaScriptCore.pbproj/project.pbxproj: Re-add -Wmissing-format-attribute.

=== Alexander-9 ===

2002-06-19  Kenneth Kocienda  <kocienda@apple.com>

        I just played alchemical voodoo games with the linker to 
        make all our frameworks and Alexander prebound.

	* JavaScriptCore.pbproj/project.pbxproj

2002-06-15  Darin Adler  <darin@apple.com>

	* JavaScriptCore.pbproj/project.pbxproj: Removed explicit PFE_FILE_C_DIALECTS now that
	Project Builder handles this automatically. Removed explicit USE_GCC3 since that's implicit
	now. Also, since this project is all C++, only use WARNING_CFLAGS with flags that are appropriate
	for C++; don't bother breaking out C vs. C++.

	* kjs/collector.cpp: Now that the system warning is fixed, use PTHREAD_MUTEX_INITIALIZER and
	PTHREAD_COND_INITIALIZER.
	* kjs/internal.cpp: Use PTHREAD_MUTEX_INITIALIZER.
	* kjs/ustring.cpp: Use PTHREAD_ONCE_INIT.

2002-06-15  Maciej Stachowiak  <mjs@apple.com>

        Made Development build mode mean what Unoptimized used to mean. Removed Unoptimized build mode. 
        Added a Mixed build mode which does what Deployment used to. All this to fix:
        
        Radar 2955367 - Change default build style to "Unoptimized"
        
	* JavaScriptCore.pbproj/project.pbxproj:

2002-06-12  Darin Adler  <darin@apple.com>

	* kjs/nodes.cpp: (Node::finalCheck): A bit of APPLE_CHANGES so we
	can compile with KJS_DEBUG_MEM defined if we want to.

2002-06-10  Darin Adler  <darin@apple.com>

	Merged in changes from KDE 3.0.1.

	* kjs/collector.cpp:
	* kjs/date_object.cpp:
	* kjs/function.cpp:
	* kjs/internal.cpp:
	* kjs/lookup.h:
	* kjs/object.cpp:
	* kjs/operations.cpp:
	* kjs/regexp.cpp:
	* kjs/regexp_object.cpp:
	* kjs/regexp_object.h:
	* kjs/string_object.cpp:
	* kjs/testkjs.cpp:
	* kjs/ustring.cpp:
	* kjs/value.cpp:
	* kjs/value.h:
	Do the merge, and add APPLE_CHANGES as needed to make things compile.

	* kjs/date_object.lut.h: Re-generated.

2002-06-07  Darin Adler  <darin@apple.com>

	* Makefile.am: Use new shared "embed.am" file so we don't need four copies of
	the embedding rules for WebFoundation, JavaScriptCore, WebCore, and WebKit.

2002-06-07  Darin Adler  <darin@apple.com>

	* JavaScriptCore.pbproj/project.pbxproj: Don't use any warning flags for C that won't work
	for C++, because PFE uses the C warning flags on a C++ compile.

=== Alexander-8 ===

2002-06-06  Darin Adler  <darin@apple.com>

	* JavaScriptCore.pbproj/project.pbxproj: Update warning flags for compatibility
	with new C++.

2002-06-05  Darin Adler  <darin@apple.com>

	Fix problem seen as build failure on Jersey.

	* Makefile.am: JavaScriptCore-stamp needs to be a dependency, not a
	source file, because it doesn't have a corresponding object file.
	Making it a dependency causes things to compile in the right order.

2002-06-04  Darin Adler  <darin@apple.com>

	Improve the speed of the JavaScript string append operation by growing
	the capacity so we don't need to reallocate the string every time.

	Also fix script execution so it doesn't use recursion to advance from
	one statement to the next, using iteration instead.

	* Makefile.am: Stop using BUILT_SOURCES to build JavaScriptCore-stamp,
	because this causes the Project Builder project to build *before* the
	subdir. Intead, use an all-am rule in a way more similar to all our
	other directories.

	* kjs/grammar.y: Link the SourceElementsNode in the opposite direction,
	so we can walk the list and execute each element instead of using
	recursion to reverse the list.
	* kjs/grammar.cpp: Check in new generated file.

	* kjs/nodes.cpp:
	(SourceElementsNode::execute):
	(SourceElementsNode::processFuncDecl):
	(SourceElementsNode::processVarDecls):
	Use loops instead of recursion.

	* kjs/ustring.h: Don't initialize all UChar objects to 0. This was
	wasting a *huge* amount of time.
	* kjs/ustring.cpp:
	(UString::Rep::create): Add a "capacity" along with the length.
	(UString::append): Include 50% extra capacity when appending.
	(UString::operator=): Reuse the buffer if possible rather than
	always creating a new one.

2002-06-02  Darin Adler  <darin@apple.com>

	* COPYING.LIB: Fix line endings. It was using CRs.

2002-05-31  Darin Adler  <darin@apple.com>

	* Makefile.am:
	* kjs/Makefile.am:
	Slight improvements to rules that touch stamp files.

2002-05-28  Maciej Stachowiak  <mjs@apple.com>

	* THANKS: Demangled.

=== Alexander-7 ===

2002-05-24  Maciej Stachowiak  <mjs@apple.com>

	Added license and acknowledgements.

	* AUTHORS: Added.
	* COPYING.LIB: Added.
	* THANKS: Added.

=== 0.3 ===

=== Alexander-6 ===

=== Alexander-5 ===

=== Alexander-4 ===

=== JavaScriptCore-5 ===

2002-05-21  Maciej Stachowiak  <mjs@apple.com>

	Reviewed by: Richard Williamson

	Fixed Radar 2928775 - Sherlock crashes sitting in stocks channel

	* kjs/internal.cpp:
	(InterpreterImp::InterpreterImp): Set the interp pointer earlier,
	in case garbage collection takes place while creating the global
	values.

2002-05-15  Darin Adler  <darin@apple.com>

	Reviewed by: Maciej Stachowiak
	
	* Makefile.am:
	Use all-am and clean-am instead of all and clean because it's better and
	to make "make check" at the top level work right.

2002-05-13  Darin Adler  <darin@apple.com>

	Reviewed by: Maciej Stachowiak

	* kjs/value.h: Fix comment typos.

=== JavaScriptCore-4 ===

2002-05-10  Maciej Stachowiak  <mjs@apple.com>

	Reviewed by: Ken Kocienda and Darin Adler

	Fixed the following bug:

	Radar 2890573 - JavaScriptCore needs to be thread-safe

	Actually this is only a weak form of thread-safety - you can safely
	use different interpreters from different threads at the same
	time. If you try to use a single interpreter object from multiple
	threads, you need to provide your own locking.

	* kjs/collector.h, kjs/collector.cpp:
	(Collector::lock, Collector::unlock): Trivial implementation of a
	recursive mutex.
	(Collector::allocate): Lock around the body of this function.
	(Collector::collect): Likewise.
	(Collector::finalCheck): Likewise.
	(Collector::numInterpreters): Likewise.
	(Collector::numGCNotAllowedObjects): Likewise.
	(Collector::numReferencedObjects): Likewise.
	* kjs/internal.cpp:
	(Parser::parse): use a mutex to lock around the whole parse, since
	it uses a bunch of global state.
	(InterpreterImp::InterpreterImp): Grab the Collector lock here,
	both the mutually exclude calls to the body of this function, and
	to protect the s_hook static member which the collector pokes at.
	(InterpreterImp::clear): Likewise.
	* kjs/ustring.cpp:
	(statBufferKeyCleanup, statBufferKeyInit, UString::ascii): Convert
	use of static variable
	* kjs/value.cpp:
	(ValueImp::ValueImp, ValueImp::mark, ValueImp::marked,
	ValueImp::setGcAllowed): Grab the GC lock around any flag changes.

=== Alexander-3 ===

2002-05-08  Darin Adler  <darin@apple.com>

	* kjs/collector.h:
	* kjs/collector.cpp:
	(Collector::numInterpreters):
	(Collector::numGCNotAllowedObjects):
	(Collector::numReferencedObjects):
	Add three new functions so we can see a bit more about leaking JavaScriptCore.

2002-05-06  Darin Adler  <darin@apple.com>

	* JavaScriptCorePrefix.h: Added.
	* JavaScriptCore.pbproj/project.pbxproj: Use PFE precompiling.
	Also switch from xNDEBUG to NDEBUG.

=== Alexander 0.3c2 (v1) ===

2002-04-18  Darin Adler  <darin@apple.com>

	* JavaScriptCore.pbproj/project.pbxproj: Oops. Take out -Wstrict-prototypes, put back
	-Wmissing-prototypes.

2002-04-18  Darin Adler  <darin@apple.com>

	* JavaScriptCore.pbproj/project.pbxproj: Take out -Wmissing-prototypes
	because system headers are triggering it when we don't have
	precompiled headers on.

2002-04-18  Darin Adler  <darin@apple.com>

	Reviewed by Maciej

	* JavaScriptCore.pbproj/project.pbxproj: Turn on gcc3 and the same set of warnings
	as in the rest of Labyrinth (see top level ChangeLog for details).

2002-04-17  Maciej Stachowiak  <mjs@apple.com>

	Reviewed by: Darin Adler  <darin@apple.com>

	* kjs/testkjs.cpp: Don't include <iostream.h> to avoid gcc3
	warning.

2002-04-15  Darin Adler  <darin@apple.com>

	Reviwed by: Maciej Stachowiak  <mjs@apple.com>

	* kjs/internal.cpp:
	* kjs/property_map.cpp:
	* kjs/ustring.h:
	Removed some unneeded <config.h> includes so we are more similar
	to the real KDE sources.

2002-04-15  Darin Adler  <darin@apple.com>

	Reviwed by: Maciej Stachowiak  <mjs@apple.com>

	Merged changes from KDE 3.0 final and did some build fixes.

	* JavaScriptCore.pbproj/project.pbxproj: Added nodes2string.cpp.

	* kjs/grammar.*: Regenerated.
	* kjs/*.lut.h: Regenerated.

2002-04-08  Darin Adler  <darin@apple.com>

	Reviwed by: Maciej Stachowiak  <mjs@apple.com>

	* JavaScriptCore.pbproj/project.pbxproj: Re-added -Wno-format-y2k.

2002-04-04  Darin Adler  <darin@apple.com>

	* JavaScriptCore.pbproj/project.pbxproj: Add an Unoptimized build
	style: exactly like Development except without the -O.

2002-04-03  Darin Adler  <darin@apple.com>

	* kjs/Makefile.am: Gratuitous cleanup.

2002-04-02  Darin Adler  <darin@apple.com>

	* JavaScriptCore.pbproj/project.pbxproj: Update flags as I did for
	WebFoundation.

2002-04-02  Maciej Stachowiak  <mjs@apple.com>

	* JavaScriptCore.pbproj/project.pbxproj: Pass -Wno-format-y2k so
	the project builds with gcc3.
	
	* kjs/nodes.cpp: Avoid including an obsolete header to avoid
	warning with gcc3.

2002-04-02  Darin Adler  <darin@apple.com>

	* kjs/property_map.cpp: (PropertyMap::~PropertyMap): Deallocate the
        map by calling clear so we don't leak the entire map.

2002-04-02  Darin Adler  <darin@apple.com>

	* kjs/internal.cpp: (InterpreterImp::globalClear): Add code to
        deallocate and null out emptyList, because once the last interpreter
        is destroyed there's nothing to keep it from being garbage collected.

2002-04-01  Darin Adler  <darin@apple.com>

        Got rid of KWQDef.h because it's dangerous to have two files with
        the same name and different contents.

	* JavaScriptCore.pbproj/project.pbxproj:
	* kjs/KWQDef.h: Removed.
	* kjs/ustring.h: Defines unsigned int types inline now.

2002-03-30  Maciej Stachowiak  <mjs@apple.com>

	Fixed Radar 2891272 (JavaScript crashes loading quicktime.com and
	apple.com)

	* kjs/object.cpp: (ObjectImp::~ObjectImp): Don't call setGCAlloc
	on object internals pointed to, because they may have already been
	collected by the time this object is collected, and in that case
	we would corrupt the malloc arena.

	* Makefile.am: Make the stamp file depend on all the sources and
	headers so the framework gets rebuilt properly.

	* JavaScriptCore.pbproj/project.pbxproj: Some random numbers moved
	around. No idea what I really changed.

2002-03-30  Darin Adler  <darin@apple.com>

	* kjs/grammar.y: Took out Id tag so we won't constantly need to
        update grammar.cpp.
	* kjs/grammar.cpp: Regenerated without Id tag.

	* .cvsignore: Ignore some additional autogenerated files.
	* kjs/.cvsignore: Ignore some additional autogenerated files.

2002-03-30  Maciej Stachowiak  <mjs@apple.com>

	* JavaScriptCore.pbproj/project.pbxproj: Install some of the
	headers.

2002-03-30  Maciej Stachowiak  <mjs@apple.com>

	Converted JavaScriptCore to build with Project Builder, in
	preparation for B&I submission.

	* English.lproj/InfoPlist.strings: Added.
	* JavaScriptCore.pbproj/.cvsignore: Added.
	* JavaScriptCore.pbproj/project.pbxproj: Added.
	
	* .cvsignore: Update the set of ignored things.

	* Makefile.am: Hand off to PB for the main build, but still handle
	the generated files and the test program.

	* kjs/Makefile.am: Don't build anything except the generated
	source files.

	* kjs/KWQDef.h, kjs/config.h: Added minimal versions of these
	files to get kjs to build.

	Check in all the genrated files, since Project Builder isn't up to
	the task of handling built sources:
	
	* kjs/array_object.lut.h: Added.
	* kjs/date_object.lut.h: Added.
	* kjs/grammar.cpp: Added.
	* kjs/grammar.cpp.h: Added.
	* kjs/grammar.h: Added.
	* kjs/lexer.lut.h: Added.
	* kjs/math_object.lut.h: Added.
	* kjs/number_object.lut.h: Added.
	* kjs/string_object.lut.h: Added.

	* kjs/.cvsignore: Update set of ignored things.

2002-03-28  Maciej Stachowiak  <mjs@apple.com>

	* kjs/kjs-test.chk: Update output for new test results.

2002-03-26  Maciej Stachowiak  <mjs@apple.com>

	Set up kjs to build by itself into libJavaScriptCore.dylib.
	
	* .cvsignore: Added.
	* Makefile.am: Added.
	* dummy.cpp: Added.
	* kjs/.cvsignore: Added.
