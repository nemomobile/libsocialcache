isEmpty(COMMON_PRI_INCLUDED) {
COMMON_PRI_INCLUDED = 1
DEFINES += 'PRIVILEGED_DATA_DIR=\'QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + \"/system/privileged/\"\''

}
