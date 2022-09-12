#ifndef _SERVER_H_
#define _SERVER_H_

#include <cstdint>

enum ProcessResult {
    PROCESS_SUCCESS,
    DEPOSIT_OVERFLOW,
    NEED_LOGOUT,
    ACCOUNT_SET_FAIL,
};

void account_list_store(uint8_t* account_list_data, uint64_t account_data_len);
uint64_t get_account_list_size();
void get_account_list(uint8_t* account_list_data, uint64_t account_list_size);

#endif