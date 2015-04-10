#! [0]
defineReplace(headersAndSources) {
    variable = $$1
    names = $$eval($$variable)
    headers =
    sources =

    for(name, names) {
        header = $${name}.h
        exists($$header) {
            headers += $$header
        }
        source = $${name}.cpp
        exists($$source) {
            sources += $$source
        }
    }
    return($$headers $$sources)
}
#! [0]

defineReplace(matchingFiles) {
    names = $$ARGS
    files =

    for(name, names) {
        header = $${name}.h
        source = $${name}.cpp
        exists($$header):exists($$source) {
            files += $$header
            files += $$source
        }
    }
    return($$files)
}

names = delegate model view main
message(Finding all headers and sources from the following list of names:)
message($$names)
allFiles = $$headersAndSources(names)
message(Found: $$allFiles)

message(Finding only matching headers and sources from the following list of names:)
message($$names)
matching = $$matchingFiles($$names)
message(Found: $$matching)
