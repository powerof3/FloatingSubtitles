# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO MikePopoloski/boost_unordered
    REF 375337252b02bf99d2bd9280a9cefcdf0bfbab01
    SHA512 4c4e5406cf38e49da6ac873bde7e2645447bac5f886e3e941f4a70e60b4fb6eb1b75a43f4254e6983722166ee363ec28612c31d2c2f3e9250435f2851cf121db
    HEAD_REF master
)

# Install codes
set(BOOST_UNORDERED_SOURCE	${SOURCE_PATH}/)
file(INSTALL ${BOOST_UNORDERED_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
