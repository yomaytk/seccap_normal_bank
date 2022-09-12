#include "deal_manager.h"
#include <stdio.h>
#include <cassert>
#include <cstring>
#include <string>
#include "account.h"

namespace AS = AccountSpace;

Manager::DealManager::DealManager() : current_account(NULL), account_list({}) {}

Manager::DealManager::~DealManager() { this->AccountListStoreStorage(); }

bool Manager::DealManager::AccountLogin() { return current_account != NULL; }

ProcessResult Manager::DealManager::NewAccount(const char* name, const char* password) {
    ProcessResult result;
    bool account_exist = false;
    for (auto&& account : this->account_list) {
        if (name == account->name) {
            result = this->SetCurrentAccount(account);
            if (strcmp(password, account->password.c_str()) != 0) {
                return ProcessResult::ACCOUNT_SET_FAIL;
            }
            account_exist = true;
            break;
        }
    }

    if (!account_exist) {
        result = this->SetCurrentAccount(
            AccountSpace::Account::NewAccount(std::string(name), std::string(password)));
        this->account_list.push_back(this->current_account);
        this->AccountListStoreStorage();
    }

    if (result != PROCESS_SUCCESS) {
        return ProcessResult::ACCOUNT_SET_FAIL;
    }

    return ProcessResult::PROCESS_SUCCESS;
}

ProcessResult Manager::DealManager::AccountListStoreStorage() {
    size_t all_account_data_size = 0;
    for (auto&& account : account_list) {
        all_account_data_size +=
            account->account_data_size + std::strlen(",") + std::strlen(",") + std::strlen("\n");
    }
    uint8_t* account_list_data = (uint8_t*)calloc(1, all_account_data_size);
    uint8_t* data_ptr          = account_list_data;
    // marshall account list
    for (auto&& account : account_list) {
        // copy account info
        // bytes: name,password,deposits
        std::memcpy(data_ptr, account->name.c_str(), account->name.size());
        data_ptr += account->name.size();

        *data_ptr = ',';
        data_ptr += 1;

        std::memcpy(data_ptr, account->password.c_str(), account->password.size());
        data_ptr += account->password.size();

        *data_ptr += ',';
        data_ptr += 1;

        std::memcpy(data_ptr, &account->deposits, sizeof(uint64_t));
        data_ptr += sizeof(uint64_t);

        *data_ptr += '\n';
        data_ptr += 1;
    }

    assert((uint64_t)(data_ptr - account_list_data) == all_account_data_size);

    account_list_store(account_list_data, all_account_data_size);

    return ProcessResult::PROCESS_SUCCESS;
}

ProcessResult Manager::DealManager::AccountListSetting() {
    uint64_t account_list_size = get_account_list_size();
    if (account_list_size > 0) {
        // printf("[account list get start] ");
        uint8_t* account_list_data = (uint8_t*)calloc(1, account_list_size);
        get_account_list(account_list_data, account_list_size);
        uint8_t* data_ptr = account_list_data;
        for (;;) {
            std::string name, password;
            uint64_t deposits;
            uint8_t* name_start_ptr = data_ptr;
            for (;; data_ptr++) {
                if (*data_ptr == ',') {
                    name = std::string(name_start_ptr, data_ptr);
                    data_ptr++;
                    break;
                }
            }

            uint8_t* password_start_ptr = data_ptr;
            for (;; data_ptr++) {
                if (*data_ptr == ',') {
                    password = std::string(password_start_ptr, data_ptr);
                    data_ptr++;
                    break;
                }
            }

            uint8_t* deposits_start_ptr = data_ptr;
            deposits                    = *(uint64_t*)deposits_start_ptr;
            data_ptr += sizeof(uint64_t);

            assert(*data_ptr++ == '\n');

            this->account_list.push_back(new AS::Account(name, password, deposits));

            if (data_ptr - account_list_data == account_list_size) {
                break;
            }
        }
    }
}

ProcessResult Manager::DealManager::SetCurrentAccount(AS::Account* account) {
    this->current_account = account;
    return ProcessResult::PROCESS_SUCCESS;
}

uint64_t Manager::DealManager::DealMyDeposit(uint64_t amount) {
    this->current_account->Deposit(amount);
    this->AccountListStoreStorage();
    return this->current_account->deposits;
}

uint64_t Manager::DealManager::DealMyWithdraw(uint64_t amount) {
    this->current_account->Withdraw(amount);
    this->AccountListStoreStorage();
    return this->current_account->deposits;
}