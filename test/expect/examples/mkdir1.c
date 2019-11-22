/* 
 * Define more restrictive permissions for 
 * new directories. Undefine the macro first
 * to avoid a warning message.
 */
#undef CTALK_DIRECTORY_MODE
#define CTALK_DIRECTORY_MODE 0700

int main () {

  DirectoryStream new thisDir;

  thisDir mkDir "testDir";

}
