TEMPLATE = subdirs

SUBDIRS *= sqldrivers
!winrt:qtHaveModule(network): SUBDIRS += bearer
qtHaveModule(gui): SUBDIRS *= imageformats platforms platforminputcontexts platformthemes generic
qtHaveModule(widgets): SUBDIRS += accessible

!winrt:!wince*:qtHaveModule(widgets):SUBDIRS += printsupport
