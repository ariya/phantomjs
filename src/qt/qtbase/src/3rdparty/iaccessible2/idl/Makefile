
MIDL        = midl
MIDL_OPTS   = /out ..\generated

first: all

all: x86 amd64

MIDL_OPTS   = /out ..\generated\x86 /env win32

x86:
	-del ..\generated\x86\dlldata.c
	-md ..\generated\x86
	$(MIDL) $(MIDL_OPTS) Accessible2.idl
	$(MIDL) $(MIDL_OPTS) AccessibleAction.idl
	$(MIDL) $(MIDL_OPTS) AccessibleApplication.idl
	$(MIDL) $(MIDL_OPTS) AccessibleComponent.idl
	$(MIDL) $(MIDL_OPTS) AccessibleEditableText.idl
	$(MIDL) $(MIDL_OPTS) AccessibleEventID.idl
	$(MIDL) $(MIDL_OPTS) AccessibleHyperlink.idl
	$(MIDL) $(MIDL_OPTS) AccessibleHypertext.idl
	$(MIDL) $(MIDL_OPTS) AccessibleImage.idl
	$(MIDL) $(MIDL_OPTS) AccessibleRelation.idl
	$(MIDL) $(MIDL_OPTS) AccessibleRole.idl
	$(MIDL) $(MIDL_OPTS) AccessibleStates.idl
	$(MIDL) $(MIDL_OPTS) AccessibleTable.idl
	$(MIDL) $(MIDL_OPTS) AccessibleTable2.idl
	$(MIDL) $(MIDL_OPTS) AccessibleTableCell.idl
	$(MIDL) $(MIDL_OPTS) AccessibleText.idl
	$(MIDL) $(MIDL_OPTS) AccessibleValue.idl
	$(MIDL) $(MIDL_OPTS) IA2CommonTypes.idl
	$(MIDL) $(MIDL_OPTS) IA2TypeLibrary.idl

MIDL_OPTS   = /out ..\generated\amd64 /env amd64

amd64:
	-del ..\generated\amd64\dlldata.c
	-md ..\generated\amd64
	$(MIDL) $(MIDL_OPTS) Accessible2.idl
	$(MIDL) $(MIDL_OPTS) AccessibleAction.idl
	$(MIDL) $(MIDL_OPTS) AccessibleApplication.idl
	$(MIDL) $(MIDL_OPTS) AccessibleComponent.idl
	$(MIDL) $(MIDL_OPTS) AccessibleEditableText.idl
	$(MIDL) $(MIDL_OPTS) AccessibleEventID.idl
	$(MIDL) $(MIDL_OPTS) AccessibleHyperlink.idl
	$(MIDL) $(MIDL_OPTS) AccessibleHypertext.idl
	$(MIDL) $(MIDL_OPTS) AccessibleImage.idl
	$(MIDL) $(MIDL_OPTS) AccessibleRelation.idl
	$(MIDL) $(MIDL_OPTS) AccessibleRole.idl
	$(MIDL) $(MIDL_OPTS) AccessibleStates.idl
	$(MIDL) $(MIDL_OPTS) AccessibleTable.idl
	$(MIDL) $(MIDL_OPTS) AccessibleTable2.idl
	$(MIDL) $(MIDL_OPTS) AccessibleTableCell.idl
	$(MIDL) $(MIDL_OPTS) AccessibleText.idl
	$(MIDL) $(MIDL_OPTS) AccessibleValue.idl
	$(MIDL) $(MIDL_OPTS) IA2CommonTypes.idl
	$(MIDL) $(MIDL_OPTS) IA2TypeLibrary.idl

