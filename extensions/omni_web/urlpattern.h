bool matches(char *pattern_data, size_t pattern_size, char *input_data, size_t input_size,
             char *baseurl_data, size_t baseurl_size);

bool match_resultset(ReturnSetInfo *rsinfo, char *pattern_data, size_t pattern_size,
                     char *input_data, size_t input_size, char *baseurl_data, size_t baseurl_size);