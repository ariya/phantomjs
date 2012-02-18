#!/bin/sh

rsync -aq --exclude ".svn" --exclude ".DS_Store" "$SRCROOT/../ForwardingHeaders" "$BUILT_PRODUCTS_DIR/$PRIVATE_HEADERS_FOLDER_PATH"
rsync -aq --exclude ".svn" --exclude ".DS_Store" "$SRCROOT/../icu" "$BUILT_PRODUCTS_DIR/$PRIVATE_HEADERS_FOLDER_PATH"
