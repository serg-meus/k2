#include "utils.h"


std::vector<std::string> split(const std::string &str, const char sep,
                                      const size_t max_split) {
    std::vector<std::string> ans;
    size_t start, end = 0, split_cr = 0;
    start = str.find_first_not_of(sep);
    while((end = str.find(sep, start)) != std::string::npos)
    {
        if(max_split > 0 && split_cr >= max_split)
            break;
        split_cr++;
        ans.push_back(str.substr(start, end - start));
        start = str.find_first_not_of(sep, end + 1);
        if(start == std::string::npos)
            break;
    }
    if(start < str.size())
        ans.push_back(str.substr(start));
    return ans;
}


std::string coord_to_str(const u8 coord) {
    auto col = get_col(coord);
    auto row = get_row(coord);
    std::string ans;
    ans.push_back(char('a' + col));
    ans.push_back(char('1' + row));
    return ans;
}
