#
#  W A R N I N G
#  -------------
#
# This file is not part of the Qt API.  It exists purely as an
# implementation detail.  It may change from version to version
# without notice, or even be removed.
#
# We mean it.
#

load(moc)
qtPrepareTool(QMAKE_QDBUSXML2CPP, qdbusxml2cpp)

defineReplace(qdbusOutputBasename) {
    return($$lower($$section($$list($$basename(1)),.,-2,-2)))
}

dbus_TYPE = $$upper($$dbus_type)

groups =
for(entry, DBUS_$${dbus_TYPE}S) {

    files = $$eval($${entry}.files)
    isEmpty(files) {
        files = $$entry
        group = dbus_$${dbus_type}
    } else {
        group = $${entry}_dbus_$${dbus_type}
    }
    groups *= $$group

    input_list = $$upper($$group)_LIST
    for(subent, $$list($$unique(files))) {

        !contains(subent, .*\\w\\.xml$) {
            warning("Invalid D-BUS $${dbus_type}: '$$subent', please use 'com.mydomain.myinterface.xml' instead.")
            next()
        }

        $$input_list += $$subent
    }
}

for(group, groups) {
    GROUP = $$upper($$group)
    input_list = $${GROUP}_LIST

    # qmake does not keep empty elements in lists, so we reverse-engineer the short name
    grp = $$replace(group, _?dbus_$${dbus_type}\$, )
    isEmpty(grp) {
        hdr_flags = $$eval(QDBUSXML2CPP_$${dbus_TYPE}_HEADER_FLAGS)
        src_flags = $$eval(QDBUSXML2CPP_$${dbus_TYPE}_SOURCE_FLAGS)
    } else {
        hdr_flags = $$eval($${grp}.header_flags)
        src_flags = $$eval($${grp}.source_flags)
    }

    $${group}_header.commands = $$QMAKE_QDBUSXML2CPP $$hdr_flags $$qdbusxml2cpp_option ${QMAKE_FILE_OUT}: ${QMAKE_FILE_IN}
    $${group}_header.output = ${QMAKE_FUNC_FILE_IN_qdbusOutputBasename}_$${dbus_type}.h
    $${group}_header.name = DBUSXML2CPP $${dbus_TYPE} HEADER ${QMAKE_FILE_IN}
    $${group}_header.variable_out = $${GROUP}_HEADERS
    $${group}_header.input = $$input_list

    $${group}_source.commands = $$QMAKE_QDBUSXML2CPP -i ${QMAKE_FILE_OUT_BASE}.h $$src_flags $$qdbusxml2cpp_option :${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
    $${group}_source.output = ${QMAKE_FUNC_FILE_IN_qdbusOutputBasename}_$${dbus_type}.cpp
    $${group}_source.name = DBUSXML2CPP $${dbus_TYPE} SOURCE ${QMAKE_FILE_IN}
    $${group}_source.variable_out = SOURCES
    $${group}_source.input = $$input_list
    $${group}_source.depends = $$eval($${group}_header.output)   # this actually belongs to the object file

    $${group}_moc.commands = $$moc_header.commands
    $${group}_moc.output = $$moc_header.output
    $${group}_moc.input = $${GROUP}_HEADERS
    $${group}_moc.variable_out = GENERATED_SOURCES
    $${group}_moc.name = $${GROUP}_$$moc_header.name

    QMAKE_EXTRA_COMPILERS += $${group}_header $${group}_source $${group}_moc
}
