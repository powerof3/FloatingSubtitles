# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO MikePopoloski/boost_unordered
    REF d6db3d2473cda6a2db13745428611d5ee36dc318
    SHA512 03102c5d5e14bb4effb60cdcd83a77f41cc21dbbf45b02b430cb276d0fdcbf271be044a90aa5d248261b8ee1ea7ac5ec0699dc1dbb7bb96f8d9cd47c1bd053e7
    HEAD_REF master
)

# Install codes
set(BOOST_UNORDERED_SOURCE	${SOURCE_PATH}/)
file(INSTALL ${BOOST_UNORDERED_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
