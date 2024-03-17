/*
 * @Author: WhiteCells
 * @Date: 2024-03-15 10:30:11
 * @Last Modified by: WhiteCells
 * @Last Modified time: 2024-03-15 10:30:11
 * @Description: 
*/

#ifndef _HTTP_RESPONSE_H_
#define _HTTP_RESPONSE_H_

#include "../buffer/buffer.h"
#include "../log/log.h"
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void init(const std::string &src_dir,
              std::string &path,
              bool is_keep_alive = false,
              int code = -1);
    void makeResponse(Buffer &buff);
    void unmapFile();
    char *file();
    size_t fileLen() const;
    void errorContent(Buffer &buff, std::string message);
    int code() const;

private:
    void addStateLine_(Buffer &buff);
    void addHeader_(Buffer &buff);
    void addContent_(Buffer &buff);

    void errorHtml_();
    std::string getFileType_();

    int code_;
    bool is_keep_alive_;
    std::string path_;
    std::string src_dir_;
    char *mm_file_;
    struct stat mm_file_stat_;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;
};

#endif // _HTTP_RESPONSE_H_