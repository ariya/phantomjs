#! [0]
DEST = "Program Files"
#! [0]
count(DEST, 1) {
    message(Only one item found in DEST.)
} else {
    message(More than one item found in DEST.)
}
