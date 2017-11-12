//
// Created by zhaoshangming on 17-10-25.
//

#ifndef FTP_DEFS_H
#define FTP_DEFS_H

#define MAX_MSG_LENGTH  9000
#define MAX_LOG_LENGTH  500
#define SERVER_HOST_DEFAULT "127.0.0.1"
#define SERVER_CMD_PORT_DEFAULT 21
#define SERVER_FILE_PORT_DEFAULT 8000
#define SERVER_ROOT_DEFAULT "/Users/zhaoshangming"
#define SERVER_NAME_PREFIX_DEFAULT "/"
#define MSG220 "220 Anonymous FTP server ready. Default dir: %s.\r\n"
#define MSG331 "331 Guest login ok, please send your complete e-mail address as password.\r\n"
const char* MSG230[] = {"230-\r\n", "230-Welcome to\r\n", "230-School of Software\r\n", "230-FTP Archives at ftp.ssast.org\r\n",
    "230-\r\n", "230 Guest login ok, access restrictions apply.\r\n"};
#define MSG200PORT "200 Port mode established.\r\n"
#define MSG150FILE "150 Opening BINARY mode data connection for "
#define MSG226 "226 Transfer complete.\r\n"
#define MSG215 "215 UNIX Type: L8\r\n"
#define MSG200TYPE "200 Type set to I.\r\n"
#define MSG226LIST "226 List finished.\r\n"
#define MSG425 "425 No connection established.\r\n"
#define MSG426 "426 Unable to write file %s to client.\r\n"
#define MSG451 "451 Error in reading file %s from disk.\r\n"
#define MSG451WRITE "451 Error in writing file %s.\r\n"
#define MSG451LIST "451 Error in listing dir %s.\r\n"
#define MSG426LIST "426 Uable to write dir %s\'s info to client.\n"
#define WRITE_ERROR "Error in writing to client in %s.\n"
const char* MSG221[] = {"221-Thank you for using FTP service ftp.ssast.org\r\n", "221 Goodbye.\r\n"};
#define CHUNK 1024
#endif //FTP_DEFS_H
