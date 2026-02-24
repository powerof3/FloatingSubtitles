# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO powerof3/CLibUtil
    REF ba29f09ee9a487190695ed379f3205a7e1c5b5ce
    SHA512 5cca66477fe247299ebec4236ed9a4db542b52a5c785c464bd53ebd136a0a59a42f610a1205c4e5c4025c70eb28f5cc79270ddb212e406894339644ff7d2fcc0
    HEAD_REF master
)

# Install codes
set(CLIBUTIL_SOURCE	${SOURCE_PATH}/include/ClibUtil)
file(INSTALL ${CLIBUTIL_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
